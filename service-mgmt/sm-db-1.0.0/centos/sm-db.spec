Summary: Service Management Databases
Name: sm-db
Version: 1.0.0
Release: %{tis_patch_ver}%{?_tis_dist}
License: Apache-2.0
Group: base
Packager: Wind River <info@windriver.com>
URL: unknown
Source0: %{name}-%{version}.tar.gz
Source1: LICENSE

BuildRequires: gcc
BuildRequires: sm-common-dev
BuildRequires: glib2-devel
BuildRequires: glibc
BuildRequires: sqlite-devel
BuildRequires: libuuid-devel

%description
Service Managment Databases

%package dev
Summary: Service Management Databases - Development files
Group: devel
Requires: %{name} = %{version}-%{release}

%description dev
Service Managment Databases  This package contains symbolic links, header
files, and related items necessary for software development.

%prep

%autosetup

%build
sqlite3 database/sm.db < database/create_sm_db.sql
sqlite3 database/sm.hb.db < database/create_sm_hb_db.sql
VER=%{version}
MAJOR=`echo $VER | awk -F . '{print $1}'`
make  VER=${VER} VER_MJR=$MAJOR

%install
rm -rf $RPM_BUILD_ROOT 
VER=%{version}
MAJOR=`echo $VER | awk -F . '{print $1}'`
make DEST_DIR=$RPM_BUILD_ROOT VER=$VER VER_MJR=$MAJOR install

%files
%license LICENSE
%defattr(-,root,root,-)
"/usr/lib64/libsm_db.so.1"
"/usr/lib64/libsm_db.so.1.0.0"
%config(noreplace)"/var/lib/sm/sm.hb.db"
%config(noreplace)"/var/lib/sm/sm.db"
"/var/lib/sm/patches/sm-patch.sql"

%files dev
%defattr(-,root,root,-)
/usr/lib64/libsm_db.so
/usr/include/*.h
