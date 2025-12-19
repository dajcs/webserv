
- test `nginx` config file
```bash
sudo nginx -t -c $(pwd)/test_nginx.conf
```

- start `nginx` with custom config
```bash
sudo nginx -c $(pwd)/test_nginx.conf
```

- reload `nginx` with new config
```bash
sudo nginx -s reload -c $(pwd)/test_nginx.conf
```

- check if `nginx` is running
```bash
# check master process and worker processes
ps aux | grep nginx
# or check open ports
sudo netstat -tuln | grep nginx
ss -tuln | grep nginx
```


- stop `nginx`
```bash
sudo nginx -s stop 
```
