====================================================
STX-HA Services API v1
====================================================

Manage HA Services running on the StarlingX OpenStack Controller cluster.
This includes services, service nodes and service groups.

The typical port used for the STX-HA Services REST API is 7777. However, proper
technique would be to look up the sysinv service endpoint in Keystone.

-------------
API versions
-------------

**************************************************************************
Lists information about all STX-HA Services API versions
**************************************************************************

.. rest_method:: GET /

**Normal response codes**

200, 300

**Error response codes**

computeFault (400, 500, ...), serviceUnavailable (503), badRequest (400),
unauthorized (401), forbidden (403), badMethod (405), overLimit (413),
itemNotFound (404)

::

   {
      "default_version":{
         "id":"v1",
         "links":[
            {
               "href":"http://128.224.150.54:7777/v1/",
               "rel":"self"
            }
         ]
      },
      "versions":[
         {
            "id":"v1",
            "links":[
               {
                  "href":"http://128.224.150.54:7777/v1/",
                  "rel":"self"
               }
            ]
         }
      ],
      "description":"STX-HA Services API allows for the management of HA Services running on the Controller Cluster.  This includes inventory collection and configuration of hosts, ports, interfaces, CPUs, disk, memory, and system configuration.  The API also supports the configuration of the cloud's SNMP interface. ",
      "name":"STX-HA Services API"
   }

This operation does not accept a request body.

*******************************************
Shows details for STX-HA Services API v1
*******************************************

.. rest_method:: GET /v1

**Normal response codes**

200, 203

**Error response codes**

computeFault (400, 500, ...), serviceUnavailable (503), badRequest (400),
unauthorized (401), forbidden (403), badMethod (405), overLimit (413),
itemNotFound (404)

::

   {
      "ihosts":[
         {
            "href":"http://128.224.150.54:7777/v1/ihosts/",
            "rel":"self"
         },
         {
            "href":"http://128.224.150.54:7777/ihosts/",
            "rel":"bookmark"
         }
      ],
      "media_types":[
         {
            "base":"application/json",
            "type":"application/vnd.openstack.sysinv.v1+json"
         }
      ],
      "links":[
         {
            "href":"http://128.224.150.54:7777/v1/",
            "rel":"self"
         },
         {
            "href":"http://www.windriver.com/developer/sysinv/dev/api-spec-v1.html",
            "type":"text/html",
            "rel":"describedby"
         }
      ],
      "inode":[
         {
            "href":"http://128.224.150.54:7777/v1/inode/",
            "rel":"self"
         },
         {
            "href":"http://128.224.150.54:7777/inode/",
            "rel":"bookmark"
         }
      ],
      "imemory":[
         {
            "href":"http://128.224.150.54:7777/v1/imemory/",
            "rel":"self"
         },
         {
            "href":"http://128.224.150.54:7777/imemory/",
            "rel":"bookmark"
         }
      ],
      "idns":[
         {
            "href":"http://128.224.150.54:7777/v1/idns/",
            "rel":"self"
         },
         {
            "href":"http://128.224.150.54:7777/idns/",
            "rel":"bookmark"
         }
      ],
      "iuser":[
         {
            "href":"http://128.224.150.54:7777/v1/iuser/",
            "rel":"self"
         },
         {
            "href":"http://128.224.150.54:7777/iuser/",
            "rel":"bookmark"
         }
      ],
      "itrapdest":[
         {
            "href":"http://128.224.150.54:7777/v1/itrapdest/",
            "rel":"self"
         },
         {
            "href":"http://128.224.150.54:7777/itrapdest/",
            "rel":"bookmark"
         }
      ],
      "istorconfig":[
         {
            "href":"http://128.224.150.54:7777/v1/istorconfig/",
            "rel":"self"
         },
         {
            "href":"http://128.224.150.54:7777/istorconfig/",
            "rel":"bookmark"
         }
      ],
      "iextoam":[
         {
            "href":"http://128.224.150.54:7777/v1/iextoam/",
            "rel":"self"
         },
         {
            "href":"http://128.224.150.54:7777/iextoam/",
            "rel":"bookmark"
         }
      ],
      "intp":[
         {
            "href":"http://128.224.150.54:7777/v1/intp/",
            "rel":"self"
         },
         {
            "href":"http://128.224.150.54:7777/intp/",
            "rel":"bookmark"
         }
      ],
      "isystems":[
         {
            "href":"http://128.224.150.54:7777/v1/isystems/",
            "rel":"self"
         },
         {
            "href":"http://128.224.150.54:7777/isystems/",
            "rel":"bookmark"
         }
      ],
      "iprofile":[
         {
            "href":"http://128.224.150.54:7777/v1/iprofile/",
            "rel":"self"
         },
         {
            "href":"http://128.224.150.54:7777/iprofile/",
            "rel":"bookmark"
         }
      ],
      "icpu":[
         {
            "href":"http://128.224.150.54:7777/v1/icpu/",
            "rel":"self"
         },
         {
            "href":"http://128.224.150.54:7777/icpu/",
            "rel":"bookmark"
         }
      ],
      "icommunity":[
         {
            "href":"http://128.224.150.54:7777/v1/icommunity/",
            "rel":"self"
         },
         {
            "href":"http://128.224.150.54:7777/icommunity/",
            "rel":"bookmark"
         }
      ],
      "iinfra":[
         {
            "href":"http://128.224.150.54:7777/v1/iinfra/",
            "rel":"self"
         },
         {
            "href":"http://128.224.150.54:7777/iinfra/",
            "rel":"bookmark"
         }
      ],
      "id":"v1",
   }

This operation does not accept a request body.

---------
Services
---------

These APIs allow the display of the services running and their
attributes

***********************************************
List all services running - STX-HA Services API
***********************************************

.. rest_method:: GET /v1/services

**Normal response codes**

200

**Error response codes**

computeFault (400, 500, ...), serviceUnavailable (503), badRequest (400),
unauthorized (401), forbidden (403), badMethod (405), overLimit (413),
itemNotFound (404)

**Response parameters**

.. csv-table::
   :header: "Parameter", "Style", "Type", "Description"
   :widths: 20, 20, 20, 60

   "services (Optional)", "plain", "xsd:list", "The list of services."
   "state (Optional)", "plain", "xsd:string", "The operational state of the service."
   "id (Optional)", "plain", "xsd:integer", "The id of the service."
   "desired_state (Optional)", "plain", "xsd:string", "The desired state of the service"
   "name (Optional)", "plain", "xsd:string", "The name of the service."
   "node_name (Optional)", "plain", "xsd:string", "The name of the host which the service is running on."

::

   {
      "services":[
         {
            "status":"",
            "state":"enabled-active",
            "id":5,
            "desired_state":"enabled-active",
            "name":"drbd-cgcs"
         },
         {
            "status":"",
            "state":"enabled-active",
            "id":3,
            "desired_state":"enabled-active",
            "name":"drbd-pg"
         },
         {
            "status":"",
            "state":"enabled-active",
            "id":4,
            "desired_state":"enabled-active",
            "name":"drbd-rabbit"
         },
         {
            "status":"",
            "state":"enabled-active",
            "id":2,
            "desired_state":"enabled-active",
            "name":"management-ip"
         },
         {
            "status":"",
            "state":"enabled-active",
            "id":1,
            "desired_state":"enabled-active",
            "name":"oam-ip"
         }
      ]
   }

This operation does not accept a request body.

****************************************************************
Shows the attributes of a specific service - STX-HA Services API
****************************************************************

.. rest_method:: GET /v1/services/​{service_id}​

**Normal response codes**

200

**Error response codes**

computeFault (400, 500, ...), serviceUnavailable (503), badRequest (400),
unauthorized (401), forbidden (403), badMethod (405), overLimit (413),
itemNotFound (404)

**Request parameters**

.. csv-table::
   :header: "Parameter", "Style", "Type", "Description"
   :widths: 20, 20, 20, 60

   "service_id", "URI", "csapi:UUID", "The unique identifier of an existing service."

**Response parameters**

.. csv-table::
   :header: "Parameter", "Style", "Type", "Description"
   :widths: 20, 20, 20, 60

   "state (Optional)", "plain", "xsd:string", "The operational state of the service."
   "id (Optional)", "plain", "xsd:integer", "The id of the service."
   "desired_state (Optional)", "plain", "xsd:string", "The desired state of the service"
   "name (Optional)", "plain", "xsd:string", "The name of the service."
   "node_name (Optional)", "plain", "xsd:string", "The name of the host which the service is running on."

::

   {
      "status":"",
      "state":"enabled-active",
      "id":1,
      "desired_state":"enabled-active",
      "name":"oam-ip"
   }

This operation does not accept a request body.

*****************************************
Modifies the configuration of a service
*****************************************

.. rest_method:: PATCH /v1/services/​{service_name}​

**Normal response codes**

200

**Error response codes**

badMediaType (415)

**Request parameters**

.. csv-table::
   :header: "Parameter", "Style", "Type", "Description"
   :widths: 20, 20, 20, 60

   "service_name", "URI", "xsd:string", "The name of an existing service."
   "enabled (Optional)", "plain", "xsd:boolean", "Service enabled."

**Response parameters**

.. csv-table::
   :header: "Parameter", "Style", "Type", "Description"
   :widths: 20, 20, 20, 60

   "enabled (Optional)", "plain", "xsd:boolean", "Service enabled."
   "name (Optional)", "plain", "xsd:string", "Service name."

::

   [
      {
         "path":"/enabled",
         "value":true,
         "op":"replace"
      }
   ]

::

   {
      "created_at":"2017-03-08T15:45:08.984813+00:00",
      "enabled":true,
      "name":"murano",
      "updated_at":null
   }

--------------
Service Nodes
--------------

These APIs allow the display of the service nodes and their attributes

**************************************
List all service nodes in the system
**************************************

.. rest_method:: GET /v1/servicenodes

**Normal response codes**

200

**Error response codes**

computeFault (400, 500, ...), serviceUnavailable (503), badRequest (400),
unauthorized (401), forbidden (403), badMethod (405), overLimit (413),
itemNotFound (404)

**Response parameters**

.. csv-table::
   :header: "Parameter", "Style", "Type", "Description"
   :widths: 20, 20, 20, 60

   "nodes (Optional)", "plain", "xsd:list", "The list of service nodes."
   "administrative_state (Optional)", "plain", "xsd:string", "Administrative state of the node."
   "ready_state (Optional)", "plain", "xsd:string", "The operational state of the node."
   "name (Optional)", "plain", "xsd:string", "The name of the node."
   "operational_state (Optional)", "plain", "xsd:string", "The operational state of the node"
   "availability_status (Optional)", "plain", "xsd:string", "The availability status of the node."
   "id (Optional)", "plain", "xsd:integer", "The id of the node."

::

   {
      "nodes":[
         {
            "administrative_state":"unlocked",
            "ready_state":"disabled",
            "name":"controller-0",
            "operational_state":"disabled",
            "availability_status":"unknown",
            "id":2
         },
         {
            "administrative_state":"unlocked",
            "ready_state":"enabled",
            "name":"controller-1",
            "operational_state":"enabled",
            "availability_status":"available",
            "id":1
         }
      ]
   }

This operation does not accept a request body.

*************************************************
Shows the attributes of a specific service node
*************************************************

.. rest_method:: GET /v1/servicenodes/​{node_id}​

**Normal response codes**

200

**Error response codes**

computeFault (400, 500, ...), serviceUnavailable (503), badRequest (400),
unauthorized (401), forbidden (403), badMethod (405), overLimit (413),
itemNotFound (404)

**Request parameters**

.. csv-table::
   :header: "Parameter", "Style", "Type", "Description"
   :widths: 20, 20, 20, 60

   "node_id", "URI", "csapi:UUID", "The unique identifier of an existing service node."

**Response parameters**

.. csv-table::
   :header: "Parameter", "Style", "Type", "Description"
   :widths: 20, 20, 20, 60

   "administrative_state (Optional)", "plain", "xsd:string", "Administrative state of the node."
   "ready_state (Optional)", "plain", "xsd:string", "The operational state of the node."
   "name (Optional)", "plain", "xsd:string", "The name of the node."
   "operational_state (Optional)", "plain", "xsd:string", "The operational state of the node"
   "availability_status (Optional)", "plain", "xsd:string", "The availability status of the node."
   "id (Optional)", "plain", "xsd:integer", "The id of the node."

::

   {
      "administrative_state":"unlocked",
      "ready_state":"enabled",
      "name":"controller-1",
      "operational_state":"enabled",
      "availability_status":"available",
      "id":1
   }

This operation does not accept a request body.

---------------
Service Groups
---------------

These APIs allow the display of the service groups and their attributes

***********************************************************
List all service groups in the system - STX-HA Services API
***********************************************************

.. rest_method:: GET /v1/servicegroup

**Normal response codes**

200

**Error response codes**

computeFault (400, 500, ...), serviceUnavailable (503), badRequest (400),
unauthorized (401), forbidden (403), badMethod (405), overLimit (413),
itemNotFound (404)

**Response parameters**

.. csv-table::
   :header: "Parameter", "Style", "Type", "Description"
   :widths: 20, 20, 20, 60

   "service_groups (Optional)", "plain", "xsd:list", "The list of service groups."
   "name (Optional)", "plain", "xsd:string", "The type of host that the service is running on."
   "service_group_name (Optional)", "plain", "xsd:string", "The name of the service group."
   "node_name (Optional)", "plain", "xsd:string", "The name of the node that the service is running on."
   "state (Optional)", "plain", "xsd:string", "The state of the service."
   "uuid (Optional)", "plain", "csapi:UUID", "The uuid of the service group."

::

   {
      "sm_servicegroup":[
         {
            "status":"",
            "name":"controller",
            "service_group_name":"web-services",
            "node_name":"controller-1",
            "state":"active",
            "desired_state":"active",
            "id":1,
            "condition":"",
            "uuid":"e3aa5e50-030b-4ab6-a339-929f0be50e5d"
         },
         {
            "status":"",
            "name":"controller",
            "service_group_name":"directory-services",
            "node_name":"controller-1",
            "state":"active",
            "desired_state":"active",
            "id":2,
            "condition":"",
            "uuid":"f7b01783-ea3d-44b8-8dd3-9a0c4a1cae9d"
         },
         {
            "status":"",
            "name":"controller",
            "service_group_name":"patching-services",
            "node_name":"controller-1",
            "state":"active",
            "desired_state":"active",
            "id":3,
            "condition":"",
            "uuid":"f64bc693-62fa-4f31-b96e-9851c42669ec"
         },
         {
            "status":"",
            "name":"controller",
            "service_group_name":"vim-services",
            "node_name":"controller-1",
            "state":"active",
            "desired_state":"active",
            "id":4,
            "condition":"",
            "uuid":"e7dab99d-7bdc-4756-b8b3-b069e7b26e0d"
         },
         {
            "status":"",
            "name":"controller",
            "service_group_name":"cloud-services",
            "node_name":"controller-1",
            "state":"active",
            "desired_state":"active",
            "id":5,
            "condition":"",
            "uuid":"149e9f4e-13ba-4d91-9e0e-09905073fda6"
         },
         {
            "status":"",
            "name":"controller",
            "service_group_name":"controller-services",
            "node_name":"controller-1",
            "state":"active",
            "desired_state":"active",
            "id":6,
            "condition":"",
            "uuid":"54d46994-9c0e-43bd-8d83-be7396f04f70"
         },
         {
            "status":"",
            "name":"controller",
            "service_group_name":"oam-services",
            "node_name":"controller-1",
            "state":"active",
            "desired_state":"active",
            "id":7,
            "condition":"",
            "uuid":"f7b532bf-0dc0-41bd-b38a-75b7747da754"
         }
      ]
   }

This operation does not accept a request body.

**********************************************************************
Shows the attributes of a specific service group - STX-HA Services API
**********************************************************************

.. rest_method:: GET /v1/servicegroup/​{servicegroup_id}​

**Normal response codes**

200

**Error response codes**

computeFault (400, 500, ...), serviceUnavailable (503), badRequest (400),
unauthorized (401), forbidden (403), badMethod (405), overLimit (413),
itemNotFound (404)

**Request parameters**

.. csv-table::
   :header: "Parameter", "Style", "Type", "Description"
   :widths: 20, 20, 20, 60

   "servicegroup_id", "URI", "csapi:UUID", "The unique identifier of an existing service group."

**Response parameters**

.. csv-table::
   :header: "Parameter", "Style", "Type", "Description"
   :widths: 20, 20, 20, 60

   "name (Optional)", "plain", "xsd:string", "The type of host that the service is running on."
   "service_group_name (Optional)", "plain", "xsd:string", "The name of the service group."
   "node_name (Optional)", "plain", "xsd:string", "The name of the node that the service is running on."
   "state (Optional)", "plain", "xsd:string", "The state of the service."
   "uuid (Optional)", "plain", "csapi:UUID", "The uuid of the service group."

::

   {
      "status":"",
      "name":"controller",
      "service_group_name":"oam-services",
      "node_name":"controller-1",
      "state":"active",
      "desired_state":"active",
      "id":7,
      "condition":"",
      "uuid":"f7b532bf-0dc0-41bd-b38a-75b7747da754"
   }

This operation does not accept a request body.









