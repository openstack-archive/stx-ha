#    Copyright 2013 IBM Corp.
#
#    Licensed under the Apache License, Version 2.0 (the "License"); you may
#    not use this file except in compliance with the License. You may obtain
#    a copy of the License at
#
#         http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
#    WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
#    License for the specific language governing permissions and limitations
#    under the License.
#
# Copyright (c) 2013-2014 Wind River Systems, Inc.
#


"""Sm common internal object model"""

import collections

from sm_api.common import exception
from sm_api.objects import utils as obj_utils
from sm_api.openstack.common import context
from sm_api.openstack.common import log as logging
from sm_api.openstack.common.rpc import common as rpc_common
from sm_api.openstack.common.rpc import serializer as rpc_serializer


LOG = logging.getLogger('object')


def get_attrname(name):
    """Return the mangled name of the attribute's underlying storage."""
    return '_%s' % name


def make_class_properties(cls):
    # NOTE(danms): Inherit Sm_apiObject's base fields only
    cls.fields.update(Sm_apiObject.fields)
    for name, typefn in cls.fields.items():

        def getter(self, name=name):
            attrname = get_attrname(name)
            if not hasattr(self, attrname):
                self.obj_load_attr(name)
            return getattr(self, attrname)

        def setter(self, value, name=name, typefn=typefn):
            self._changed_fields.add(name)
            try:
                return setattr(self, get_attrname(name), typefn(value))
            except Exception:
                attr = "%s.%s" % (self.obj_name(), name)
                LOG.exception(_('Error setting %(attr)s') %
                              {'attr': attr})
                raise

        setattr(cls, name, property(getter, setter))


class Sm_apiObjectMetaclass(type):
    """Metaclass that allows tracking of object classes."""

    # NOTE(danms): This is what controls whether object operations are
    # remoted. If this is not None, use it to remote things over RPC.
    indirection_api = None

    def __init__(cls, names, bases, dict_):
        if not hasattr(cls, '_obj_classes'):
            # This will be set in the 'Sm_apiObject' class.
            cls._obj_classes = collections.defaultdict(list)
        else:
            # Add the subclass to Sm_apiObject._obj_classes
            make_class_properties(cls)
            cls._obj_classes[cls.obj_name()].append(cls)


# These are decorators that mark an object's method as remotable.
# If the metaclass is configured to forward object methods to an
# indirection service, these will result in making an RPC call
# instead of directly calling the implementation in the object. Instead,
# the object implementation on the remote end will perform the
# requested action and the result will be returned here.
def remotable_classmethod(fn):
    """Decorator for remotable classmethods."""
    def wrapper(cls, context, *args, **kwargs):
        if Sm_apiObject.indirection_api:
            result = Sm_apiObject.indirection_api.object_class_action(
                context, cls.obj_name(), fn.__name__, cls.version,
                args, kwargs)
        else:
            result = fn(cls, context, *args, **kwargs)
            if isinstance(result, Sm_apiObject):
                result._context = context
        return result
    return classmethod(wrapper)


# See comment above for remotable_classmethod()
#
# Note that this will use either the provided context, or the one
# stashed in the object. If neither are present, the object is
# "orphaned" and remotable methods cannot be called.
def remotable(fn):
    """Decorator for remotable object methods."""
    def wrapper(self, *args, **kwargs):
        ctxt = self._context
        try:
            if isinstance(args[0], (context.RequestContext,
                                    rpc_common.CommonRpcContext)):
                ctxt = args[0]
                args = args[1:]
        except IndexError:
            pass
        if ctxt is None:
            raise exception.OrphanedObjectError(method=fn.__name__,
                                                objtype=self.obj_name())
        if Sm_apiObject.indirection_api:
            updates, result = Sm_apiObject.indirection_api.object_action(
                ctxt, self, fn.__name__, args, kwargs)
            for key, value in updates.items():
                if key in self.fields:
                    self[key] = self._attr_from_primitive(key, value)
            self._changed_fields = set(updates.get('obj_what_changed', []))
            return result
        else:
            return fn(self, ctxt, *args, **kwargs)
    return wrapper


# Object versioning rules
#
# Each service has its set of objects, each with a version attached. When
# a client attempts to call an object method, the server checks to see if
# the version of that object matches (in a compatible way) its object
# implementation. If so, cool, and if not, fail.
def check_object_version(server, client):
    try:
        client_major, _client_minor = client.split('.')
        server_major, _server_minor = server.split('.')
        client_minor = int(_client_minor)
        server_minor = int(_server_minor)
    except ValueError:
        raise exception.IncompatibleObjectVersion(
            _('Invalid version string'))

    if client_major != server_major:
        raise exception.IncompatibleObjectVersion(
            dict(client=client_major, server=server_major))
    if client_minor > server_minor:
        raise exception.IncompatibleObjectVersion(
            dict(client=client_minor, server=server_minor))


class Sm_apiObject(object):
    """Base class and object factory.

    This forms the base of all objects that can be remoted or instantiated
    via RPC. Simply defining a class that inherits from this base class
    will make it remotely instantiatable. Objects should implement the
    necessary "get" classmethod routines as well as "save" object methods
    as appropriate.
    """
    __metaclass__ = Sm_apiObjectMetaclass

    # Version of this object (see rules above check_object_version())
    version = '1.0'

    # The fields present in this object as key:typefn pairs. For example:
    #
    # fields = { 'foo': int,
    #            'bar': str,
    #            'baz': lambda x: str(x).ljust(8),
    #          }
    #
    # NOTE(danms): The base Sm_apiObject class' fields will be inherited
    # by subclasses, but that is a special case. Objects inheriting from
    # other objects will not receive this merging of fields contents.
    fields = {}
    # JKUNG until avail     'created_at': obj_utils.datetime_or_str_or_none,
    #     'updated_at': obj_utils.datetime_or_str_or_none,
    #     }
    obj_extra_fields = []

    def __init__(self):
        self._changed_fields = set()
        self._context = None

    @classmethod
    def obj_name(cls):
        """Return a canonical name for this object which will be used over
        the wire for remote hydration.
        """
        return cls.__name__

    @classmethod
    def obj_class_from_name(cls, objname, objver):
        """Returns a class from the registry based on a name and version."""
        if objname not in cls._obj_classes:
            LOG.error(_('Unable to instantiate unregistered object type '
                        '%(objtype)s') % dict(objtype=objname))
            raise exception.UnsupportedObjectError(objtype=objname)

        compatible_match = None
        for objclass in cls._obj_classes[objname]:
            if objclass.version == objver:
                return objclass
            try:
                check_object_version(objclass.version, objver)
                compatible_match = objclass
            except exception.IncompatibleObjectVersion:
                pass

        if compatible_match:
            return compatible_match

        raise exception.IncompatibleObjectVersion(objname=objname,
                                                  objver=objver)

    _attr_created_at_from_primitive = obj_utils.dt_deserializer
    _attr_updated_at_from_primitive = obj_utils.dt_deserializer

    def _attr_from_primitive(self, attribute, value):
        """Attribute deserialization dispatcher.

        This calls self._attr_foo_from_primitive(value) for an attribute
        foo with value, if it exists, otherwise it assumes the value
        is suitable for the attribute's setter method.
        """
        handler = '_attr_%s_from_primitive' % attribute
        if hasattr(self, handler):
            return getattr(self, handler)(value)
        return value

    @classmethod
    def obj_from_primitive(cls, primitive, context=None):
        """Simple base-case hydration.

        This calls self._attr_from_primitive() for each item in fields.
        """
        if primitive['sm_api_object.namespace'] != 'sm_api':
            # NOTE(danms): We don't do anything with this now, but it's
            # there for "the future"
            raise exception.UnsupportedObjectError(
                objtype='%s.%s' % (primitive['sm_api_object.namespace'],
                                   primitive['sm_api_object.name']))
        objname = primitive['sm_api_object.name']
        objver = primitive['sm_api_object.version']
        objdata = primitive['sm_api_object.data']
        objclass = cls.obj_class_from_name(objname, objver)
        self = objclass()
        self._context = context
        for name in self.fields:
            if name in objdata:
                setattr(self, name,
                        self._attr_from_primitive(name, objdata[name]))
        changes = primitive.get('sm_api_object.changes', [])
        self._changed_fields = set([x for x in changes if x in self.fields])
        return self

    _attr_created_at_to_primitive = obj_utils.dt_serializer('created_at')
    _attr_updated_at_to_primitive = obj_utils.dt_serializer('updated_at')

    def _attr_to_primitive(self, attribute):
        """Attribute serialization dispatcher.

        This calls self._attr_foo_to_primitive() for an attribute foo,
        if it exists, otherwise it assumes the attribute itself is
        primitive-enough to be sent over the RPC wire.
        """
        handler = '_attr_%s_to_primitive' % attribute
        if hasattr(self, handler):
            return getattr(self, handler)()
        else:
            return getattr(self, attribute)

    def obj_to_primitive(self):
        """Simple base-case dehydration.

        This calls self._attr_to_primitive() for each item in fields.
        """
        primitive = dict()
        for name in self.fields:
            if hasattr(self, get_attrname(name)):
                primitive[name] = self._attr_to_primitive(name)
        obj = {'sm_api_object.name': self.obj_name(),
               'sm_api_object.namespace': 'sm_api',
               'sm_api_object.version': self.version,
               'sm_api_object.data': primitive}
        if self.obj_what_changed():
            obj['sm_api_object.changes'] = list(self.obj_what_changed())
        return obj

    def obj_load_attr(self, attrname):
        """Load an additional attribute from the real object.

        This should use self._conductor, and cache any data that might
        be useful for future load operations.
        """
        raise NotImplementedError(
            _("Cannot load '%(attrname)s' in the base class") %
            {'attrname': attrname})

    def save(self, context):
        """Save the changed fields back to the store.

        This is optional for subclasses, but is presented here in the base
        class for consistency among those that do.
        """
        raise NotImplementedError('Cannot save anything in the base class')

    def obj_what_changed(self):
        """Returns a set of fields that have been modified."""
        return self._changed_fields

    def obj_reset_changes(self, fields=None):
        """Reset the list of fields that have been changed.

        Note that this is NOT "revert to previous values"
        """
        if fields:
            self._changed_fields -= set(fields)
        else:
            self._changed_fields.clear()

    # dictish syntactic sugar
    def iteritems(self):
        """For backwards-compatibility with dict-based objects.

        NOTE(danms): May be removed in the future.
        """
        for name in self.fields.keys() + self.obj_extra_fields:
            if (hasattr(self, get_attrname(name)) or
                    name in self.obj_extra_fields):
                yield name, getattr(self, name)

    items = lambda self: list(self.items())

    def __getitem__(self, name):
        """For backwards-compatibility with dict-based objects.

        NOTE(danms): May be removed in the future.
        """
        return getattr(self, name)

    def __setitem__(self, name, value):
        """For backwards-compatibility with dict-based objects.

        NOTE(danms): May be removed in the future.
        """
        setattr(self, name, value)

    def __contains__(self, name):
        """For backwards-compatibility with dict-based objects.

        NOTE(danms): May be removed in the future.
        """
        return hasattr(self, get_attrname(name))

    def get(self, key, value=None):
        """For backwards-compatibility with dict-based objects.

        NOTE(danms): May be removed in the future.
        """
        return self[key]

    def update(self, updates):
        """For backwards-compatibility with dict-base objects.

        NOTE(danms): May be removed in the future.
        """
        for key, value in updates.items():
            self[key] = value

    def as_dict(self):
        return dict((k, getattr(self, k))
                for k in self.fields
                if hasattr(self, k))

    @classmethod
    def get_defaults(cls):
        """Return a dict of its fields with their default value."""
        return dict((k, v(None))
                    for k, v in cls.fields.items()
                    if k != "id" and callable(v))


class ObjectListBase(object):
    """Mixin class for lists of objects.

    This mixin class can be added as a base class for an object that
    is implementing a list of objects. It adds a single field of 'objects',
    which is the list store, and behaves like a list itself. It supports
    serialization of the list of objects automatically.
    """
    fields = {
        'objects': list,
        }

    def __iter__(self):
        """List iterator interface."""
        return iter(self.objects)

    def __len__(self):
        """List length."""
        return len(self.objects)

    def __getitem__(self, index):
        """List index access."""
        if isinstance(index, slice):
            new_obj = self.__class__()
            new_obj.objects = self.objects[index]
            # NOTE(danms): We must be mixed in with an Sm_apiObject!
            new_obj.obj_reset_changes()
            new_obj._context = self._context
            return new_obj
        return self.objects[index]

    def __contains__(self, value):
        """List membership test."""
        return value in self.objects

    def count(self, value):
        """List count of value occurrences."""
        return self.objects.count(value)

    def index(self, value):
        """List index of value."""
        return self.objects.index(value)

    def _attr_objects_to_primitive(self):
        """Serialization of object list."""
        return [x.obj_to_primitive() for x in self.objects]

    def _attr_objects_from_primitive(self, value):
        """Deserialization of object list."""
        objects = []
        for entity in value:
            obj = Sm_apiObject.obj_from_primitive(entity,
                                                  context=self._context)
            objects.append(obj)
        return objects


class Sm_apiObjectSerializer(rpc_serializer.Serializer):
    """A Sm_apiObject-aware Serializer.

    This implements the Oslo Serializer interface and provides the
    ability to serialize and deserialize Sm_apiObject entities. Any service
    that needs to accept or return Sm_apiObjects as arguments or result values
    should pass this to its RpcProxy and RpcDispatcher objects.
    """

    def _process_iterable(self, context, action_fn, values):
        """Process an iterable, taking an action on each value.
        :param:context: Request context
        :param:action_fn: Action to take on each item in values
        :param:values: Iterable container of things to take action on
        :returns: A new container of the same type (except set) with
                  items from values having had action applied.
        """
        iterable = values.__class__
        if iterable == set:
            # NOTE(danms): A set can't have an unhashable value inside, such as
            # a dict. Convert sets to tuples, which is fine, since we can't
            # send them over RPC anyway.
            iterable = tuple
        return iterable([action_fn(context, value) for value in values])

    def serialize_entity(self, context, entity):
        if isinstance(entity, (tuple, list, set)):
            entity = self._process_iterable(context, self.serialize_entity,
                                            entity)
        elif (hasattr(entity, 'obj_to_primitive') and
                callable(entity.obj_to_primitive)):
            entity = entity.obj_to_primitive()
        return entity

    def deserialize_entity(self, context, entity):
        if isinstance(entity, dict) and 'sm_api_object.name' in entity:
            entity = Sm_apiObject.obj_from_primitive(entity, context=context)
        elif isinstance(entity, (tuple, list, set)):
            entity = self._process_iterable(context, self.deserialize_entity,
                                            entity)
        return entity


def obj_to_primitive(obj):
    """Recursively turn an object into a python primitive.

    An Sm_apiObject becomes a dict, and anything that implements ObjectListBase
    becomes a list.
    """
    if isinstance(obj, ObjectListBase):
        return [obj_to_primitive(x) for x in obj]
    elif isinstance(obj, Sm_apiObject):
        result = {}
        for key, value in obj.items():
            result[key] = obj_to_primitive(value)
        return result
    else:
        return obj
