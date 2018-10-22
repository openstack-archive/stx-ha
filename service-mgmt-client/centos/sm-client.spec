Summary: Service Management Client and CLI 
Name: sm-client
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
BuildRequires: python2-pip
BuildRequires: python2-wheel
Requires: python-libs
Requires: python-six
%prep
%setup -q

%build
%{__python2} setup.py build
%py2_build_wheel

%install
%global _buildsubdir %{_builddir}/%{name}-%{version}
%{__python2} setup.py install -O1 --skip-build --root %{buildroot}
mkdir -p $RPM_BUILD_ROOT/wheels
install -m 644 dist/*.whl $RPM_BUILD_ROOT/wheels/
install -d %{buildroot}/usr/bin
install -m 755 %{_buildsubdir}/usr/bin/smc %{buildroot}/usr/bin

%description
Service Management Client and CLI 

#%package -n sm-client-py-src-tar
#Summary: Service Management Client and CLI source tarball 
#Group: base

#%description -n sm-client-py-src-tar
#Service Management API


#%post -n sm-client-py-src-tar
## sm-client-py-src-tar - postinst
#	if [ -f $D/usr/src/sm-client-1.0.tar.bz2 ] ; then
#		( cd $D/ && tar -xf $D/usr/src/sm-client-1.0.tar.bz2 )
#	fi


%files
%defattr(-,root,root,-)
%dir "/usr/lib/python2.7/site-packages/sm_client"
/usr/lib/python2.7/site-packages/sm_client/*
"/usr/bin/smc"
%dir "/usr/lib/python2.7/site-packages/sm_client-1.0.0-py2.7.egg-info"
/usr/lib/python2.7/site-packages/sm_client-1.0.0-py2.7.egg-info/*

#%files -n sm-client-py-src-tar
#%defattr(-,-,-,-)
#"/usr/src/sm-client-1.0.tar.bz2"

%package wheels
Summary: %{module_name} wheels

%description wheels
Contains python wheels for %{module_name}

%files wheels
/wheels/*
