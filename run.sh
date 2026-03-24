make clean
make DEBUG=1

./bin/master_dns &
./bin/serveur_ftp &
./bin/client localhost

if pgrep master_dns >/dev/null 2>&1; then
	pkill -SIGINT master_dns
fi
if pgrep serveur_ftp >/dev/null 2>&1; then
	pkill -SIGINT serveur_ftp
fi
