Summary: Service Management API
Name: sm-api
Version: 1.0
Release: %{tis_patch_ver}%{?_tis_dist}
License: Apache-2.0
Group: base
Packager: Wind River <info@windriver.com>
URL: unknown
Source0:          %{name}-%{version}.tar.gz

%define debug_package %{nil}

BuildRequires: python
BuildRequires: python-setuptools
BuildRequires: util-linux
# BuildRequires systemd is to get %_unitdir I think
BuildRequires: systemd
BuildRequires: systemd-devel
Requires: python-libs
# Needed for python2 and python3 compatible
Requires: python-future
# Needed for /etc/init.d, can be removed when we go fully systemd
Requires: chkconfig
# Needed for /etc/pmon.d
Requires:  cgts-mtce-common-pmon


%prep
%setup -q

%build
%{__python2} setup.py build

%install
%global _buildsubdir %{_builddir}/%{name}-%{version}
%{__python2} setup.py install -O1 --skip-build --root %{buildroot}
install -d %{buildroot}/etc/sm
install -d %{buildroot}/etc/init.d
install -d %{buildroot}/etc/pmon.d
install -d %{buildroot}/etc/sm-api
install -d %{buildroot}%{_unitdir}
install -m 644 %{_buildsubdir}/scripts/sm_api.ini %{buildroot}/etc/sm
install -m 755 %{_buildsubdir}/scripts/sm-api %{buildroot}/etc/init.d
install -m 644 %{_buildsubdir}/scripts/sm-api.service %{buildroot}%{_unitdir}
install -m 644 %{_buildsubdir}/scripts/sm-api.conf %{buildroot}/etc/pmon.d
install -m 644 %{_buildsubdir}/etc/sm-api/policy.json %{buildroot}/etc/sm-api

%description
Service Management API

#%package -n sm-api-py-src-tar
#Summary: Service Management API
#Group: base

#%description -n sm-api-py-src-tar
#Service Management API


#%post -n sm-api-py-src-tar
## sm-api-py-src-tar - postinst
#	if [ -f $D/usr/src/sm-api-1.0.tar.bz2 ] ; then
#		( cd $D/ && tar -xf $D/usr/src/sm-api-1.0.tar.bz2 )
#	fi

%post
/usr/bin/systemctl enable sm-api.service >/dev/null 2>&1

%files
%defattr(-,root,root,-)
%dir "/usr/lib/python2.7/site-packages/sm_api"
/usr/lib/python2.7/site-packages/sm_api/*
%dir "/usr/lib/python2.7/site-packages/sm_api-1.0.0-py2.7.egg-info"
/usr/lib/python2.7/site-packages/sm_api-1.0.0-py2.7.egg-info/*
"/usr/bin/sm-api"
%dir "/etc/sm"
"/etc/init.d/sm-api"
"/etc/pmon.d/sm-api.conf"
"/etc/sm-api/policy.json"
"/etc/sm/sm_api.ini"
%{_unitdir}/*

#%files -n sm-api-py-src-tar
#%defattr(-,-,-,-)
#"/usr/src/sm-api-1.0.tar.bz2"

