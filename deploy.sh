make clean
make
make install
cp manage.service /etc/systemd/system/
systemctl enable manage
