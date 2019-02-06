Summary: Service Management Common
Name: sm-common
Version: 1.0.0
Release: %{tis_patch_ver}%{?_tis_dist}
License: Apache-2.0
Group: base
Packager: Wind River <info@windriver.com>
URL: unknown
Source0: %{name}-%{version}.tar.gz


BuildRequires: build-info-dev
BuildRequires: glib2-devel
BuildRequires: sqlite-devel
BuildRequires: gcc
BuildRequires: util-linux
BuildRequires: libuuid-devel

Requires: mtce-pmon
Requires: /bin/sh
Requires: sqlite
Requires: util-linux
Requires: systemd
# BuildRequires is to get %_unitdir I think
BuildRequires: systemd
BuildRequires: systemd-devel
Requires(post): systemd
Requires: %{name}-libs = %{version}-%{release}

%description
Service Managment Common
 
#%package -n sm-common-dbg
#Summary: Service Management Common - Debugging files
#Group: devel
##Suggests: libsqlite3-dbg
##Suggests: libgcc-s-dbg(x86_64)
##Suggests: libglib-2.0-dbg(x86_64)
##Suggests: lmapi-dbg
##Suggests: libstdc++-dbg(x86_64)
##Suggests: libc6-dbg(x86_64)
##Suggests: util-linux-libuuid-dbg
##Recommends: sm-common = 1.0.0-r7.0
#Provides: sm-common-dbg(x86_64) = 1.0.0-r7.0

#%description -n sm-common-dbg
#Service Managment Common  This package contains ELF symbols and related
#sources for debugging purposes.

%package libs
Summary: Service Management Common - shared library fles
Group: base
#Provides: sm-common-libs(x86_64) = 1.0.0-r7.0

%description libs
Service Managment Common  This package contains shared libraries.

%package dev
Summary: Service Management Common - Development files
Group: devel
Requires: %{name}-libs = %{version}-%{release}

%description dev
Service Managment Common  This package contains symbolic links, header
files, and related items necessary for software development.

%package -n sm-eru
Summary: Service Management ERU
Group: base
Requires: %{name}-libs = %{version}-%{release}

%description -n sm-eru
Service Managment ERU.

%prep
%autosetup

%build
VER=%{version}

MAJOR=`echo $VER | awk -F . '{print $1}'`
MINOR=`echo $VER | awk -F . '{print $2}'`
make  VER=${VER} VER_MJR=$MAJOR %{?_smp_mflags}

%global _buildsubdir %{_builddir}/%{name}-%{version}

%install
rm -rf %{buildroot}
VER=%{version}
MAJOR=`echo $VER | awk -F . '{print $1}'`
MINOR=`echo $VER | awk -F . '{print $2}'`
make DEST_DIR=%{buildroot} BIN_DIR=%{_bindir} UNIT_DIR=%{_unitdir} LIB_DIR=%{_libdir} INC_DIR=%{_includedir} BUILDSUBDIR=%{_buildsubdir} VER=$VER VER_MJR=$MAJOR install

%post
/usr/bin/systemctl enable sm-watchdog.service >/dev/null 2>&1

%post -n sm-eru
/usr/bin/systemctl enable sm-eru.service >/dev/null 2>&1


%files
%license LICENSE
%defattr(-,root,root,-)
/etc/init.d/sm-watchdog
/etc/pmon.d/sm-watchdog.conf
/usr/bin/sm-watchdog
/usr/lib/systemd/system/sm-watchdog.service

#%{_unitdir}/*
#%{_bindir}/*
#/etc/init.d/*
#/etc/pmon.d/*

%files libs
%{_libdir}/*.so.*
%dir "/var/lib/sm"
%dir "/var/lib/sm/watchdog"
%dir "/var/lib/sm/watchdog/modules"
/var/lib/sm/watchdog/modules/*.so.*

%files -n sm-eru
%defattr(-,root,root,-)
/etc/init.d/sm-eru
/etc/pmon.d/sm-eru.conf
/usr/bin/sm-eru
/usr/bin/sm-eru-dump
/usr/lib/systemd/system/sm-eru.service


# Commented out for now, but something seems to auto generate
# a sm-common-debuginfo RPM even without this.
#%files -n sm-common-dbg
#%defattr(-,-,-,-)
#%dir "/usr/lib64/.debug"
#"/usr/lib64/.debug/libsm_common.so.1.0.0"
#%dir "/usr/bin/.debug"
#"/usr/bin/.debug/sm-eru-dump"
#"/usr/bin/.debug/sm-watchdog"
#"/usr/bin/.debug/sm-eru"
#%dir "/usr/src/debug/sm-common"
#%dir "/usr/src/debug/sm-common/1.0.0-r7"
#%dir "/usr/src/debug/sm-common/1.0.0-r7/src"
#/usr/src/debug/sm-common/1.0.0-r7/src/*.h
#/usr/src/debug/sm-common/1.0.0-r7/src/*.c
#%dir "/var/lib/sm/watchdog/modules/.debug"
#"/var/lib/sm/watchdog/modules/.debug/libsm_watchdog_nfs.so.1.0.0"

%files dev
%defattr(-,root,root,-)
%{_includedir}/*
%{_libdir}/*.so
/var/lib/sm/watchdog/modules/libsm_watchdog_nfs.so
