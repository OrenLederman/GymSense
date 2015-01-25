# GymSense

# NodeJS installation
sudo apt-get update
sudo apt-get install nodejs
sudo apt-get install npm
sudo npm install express --save 
sudo npm install express-generator -g
sudo ln -s /usr/bin/nodejs /usr/local/bin/node

# Firewall setup
sudo apt-get update
sudo apt-get install iptables-persistent
sudo vim /etc/sysctl.conf
  uncomment "net.ipv4.ip_forward"
sudo iptables -A PREROUTING -t nat -i eth0 -p tcp --dport 80 -j REDIRECT --to-port 3000
sudo iptables -A INPUT -p tcp -m tcp --sport 80 -j ACCEPT
sudo iptables -A OUTPUT -p tcp -m tcp --dport 80 -j ACCEPT
sudo iptables-save | sudo tee /etc/iptables/rules.v4

# Install mySQL
http://www.tocker.ca/2014/04/21/installing-mysql-5-6-on-ubuntu-14-04-trusty-tahr.html
sudo apt-get install mysql-server-5.6
mysqladmin -u root password ??????
mysql -u root -p
  CREATE USER 'gymsense'@'localhost' IDENTIFIED BY '?????';
  GRANT ALL PRIVILEGES ON * . * TO 'gymsense'@'localhost';
  FLUSH PRIVILEGES;
mysql -u gymsense -p
  create database gymsense
  use gymsense
  create table sensor_data (ts TIMESTAMP,sensor_id TINYINT, activity_cnt TINYINT NOT NULL) ENGINE=innodb;
  ALTER TABLE sensor_data ADD PRIMARY KEY (ts,sensor_id);
  CREATE INDEX sensor_data_ts ON sensor_data (ts);
  CREATE INDEX sensor_data_sid ON sensor_data (sensor_id);

# For startup script, use:

