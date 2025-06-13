# A simple FTP project
## database
```mysql
# 1. create database
create database ftp;

# 2. change database
use ftp

# 3. create `users` table
CREATE TABLE `users` (
  `uid` int NOT NULL AUTO_INCREMENT,
  `username` char(12) NOT NULL,
  `passwd` char(48) NOT NULL,
  `tomb` tinyint(1) DEFAULT '0',
  PRIMARY KEY (`uid`),
  UNIQUE KEY `username` (`username`)
);

# 4. create `disk` table
# type: 'f' or 'd'
CREATE TABLE `disk` (
  `fid` int NOT NULL AUTO_INCREMENT,
  `filename` char(24) NOT NULL,
  `type` char(1) NOT NULL,
  `filepath` char(48) NOT NULL,
  `pid` int DEFAULT '-1',
  `uid` int DEFAULT NULL,
  `md5` char(32) DEFAULT NULL,
  `tomb` tinyint(1) DEFAULT '0',
  PRIMARY KEY (`fid`),
  KEY `uid` (`uid`),
  CONSTRAINT `disk_ibfk_1` FOREIGN KEY (`uid`) REFERENCES `users` (`uid`)
);
```

