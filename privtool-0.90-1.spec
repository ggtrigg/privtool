Description: Privtool a X11 sun like mail program with pgp support
Name: privtool
Version: 0.90
Vendor: David Summers <david@summersoft.fay.ar.us>
Release: 1
Source: privtool-0.90.tar.gz
Copyright: Freely Distributable
Group: Applications/Mail
Patch0: redhat.privtool.patch
Buildroot: /tmp/privtool

%prep
%setup
%patch -p1
ln -sf Makefile.linux Makefile
rm -rf linux/*.o

%build
make

%install
rm -rf /tmp/privtool
mkdir -p /tmp/privtool/usr/bin
cp privtool /tmp/privtool/usr/bin/

%files
%doc COPYING Credits Privrc.test README README.1ST user.doc INSTALL_MOTIF TODO_MOTIF
/usr/bin/privtool
