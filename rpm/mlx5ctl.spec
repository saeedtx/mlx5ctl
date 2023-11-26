Summary: ConnectX device control utility
Name: mlx5ctl
Version: %{APP_VERSION}
Release: 1
License: BSD-3-Clause
Group: Development/Tools
URL: https://github.com/saeedtx/mlx5ctl
Source0: mlx5ctl-%{APP_VERSION}.tar.gz

%description
Userspace Linux Debug Utilities for mlx5 ConnectX Devices

%define _bindir /usr/local/bin

%prep
%setup -q

%build
make

%install
make install DESTDIR=%{buildroot}

%files
%{_bindir}/mlx5ctl
#%{_mandir}/man1/mlx5ctl.1.gz  # Example man page if available
