Summary: Service Management
Name: sm
Version: 1.0.0
Release: %{tis_patch_ver}%{?_tis_dist}
License: Apache-2.0
Group: base
Packager: Wind River <info@windriver.com>
URL: unknown
Source0: %{name}-%{version}.tar.gz
Source1: LICENSE

BuildRequires: fm-common-dev
BuildRequires: sm-db-dev
BuildRequires: sm-common-dev
BuildRequires: mtce-dev
BuildRequires: glib2-devel
BuildRequires: glibc
BuildRequires: sqlite-devel
BuildRequires: gcc
BuildRequires: libuuid-devel
BuildRequires: json-c
BuildRequires: json-c-devel
BuildRequires: openssl-devel


# BuildRequires is to get %_unitdir I think
BuildRequires: systemd
BuildRequires: systemd-devel
Requires(post): systemd
Requires(preun): systemd

# Needed for /etc/init.d, can be removed when we go fully systemd
Requires: chkconfig
# Needed for /etc/pmon.d
Requires:  mtce-pmon
# Needed for /etc/logrotate.d
Requires: logrotate
# for collect stuff
Requires: time

%description
Service Managment

#%package -n sm-dbg
#Summary: Service Management - Debugging files
#Group: devel
#Recommends: sm = 1.0.0-r10.0

#%description -n sm-dbg
#Service Managment  This package contains ELF symbols and related sources
#for debugging purposes.

%prep
%autosetup

%build
VER=%{version}
MAJOR=`echo $VER | awk -F . '{print $1}'`
make -j"%(nproc)"

%install
rm -rf $RPM_BUILD_ROOT
VER=%{version}
MAJOR=`echo $VER | awk -F . '{print $1}'`
make DEST_DIR=$RPM_BUILD_ROOT UNIT_DIR=%{_unitdir} install_non_bb

%post
/usr/bin/systemctl enable sm.service >/dev/null 2>&1
/usr/bin/systemctl enable sm-shutdown.service >/dev/null 2>&1


%files
%license LICENSE
%defattr(-,root,root,-)
%{_unitdir}/*
"/usr/bin/sm"
"/usr/local/sbin/sm-notify"
"/usr/local/sbin/sm-troubleshoot"
"/usr/local/sbin/sm-notification"
"/etc/init.d/sm"
"/etc/init.d/sm-shutdown"
"/etc/pmon.d/sm.conf"
"/etc/logrotate.d/sm.logrotate"

#%files -n sm-dbg
#%defattr(-,-,-,-)
#%dir "/usr"
#%dir "/usr/bin"
#%dir "/usr/src"
#%dir "/usr/bin/.debug"
#"/usr/bin/.debug/sm"
#%dir "/usr/src/debug"
#%dir "/usr/src/debug/sm"
#%dir "/usr/src/debug/sm/1.0.0-r10"
#%dir "/usr/src/debug/sm/1.0.0-r10/src"
#/usr/src/debug/sm/1.0.0-r10/src/*.c
#/usr/src/debug/sm/1.0.0-r10/src/*.h


