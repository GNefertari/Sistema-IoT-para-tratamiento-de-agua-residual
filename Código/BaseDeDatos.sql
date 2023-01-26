CREATE SCHEMA IF NOT EXISTS `mydb`;
USE `mydb` ;


CREATE TABLE `rapidez` (
  `idrapidez` int NOT NULL AUTO_INCREMENT,
  `nombre` varchar(100) DEFAULT NULL,
  `velocidad` int DEFAULT NULL,
  PRIMARY KEY (`idrapidez`),
  UNIQUE KEY `idrapidez_UNIQUE` (`idrapidez`)
);


LOCK TABLES `rapidez` WRITE;
INSERT INTO `rapidez` VALUES (1,'Lento',1),(2,'Medio',2),(3,'Rapido',3);
UNLOCK TABLES;


CREATE TABLE `configuracion` (
  `idConfiguracion` int NOT NULL AUTO_INCREMENT,
  `Rotor_Tiempo` int DEFAULT NULL,
  `Tiempo_Reposo` int DEFAULT NULL,
  `Cantidad_Polimero` int DEFAULT NULL,
  `idrapidez` int DEFAULT NULL,
  PRIMARY KEY (`idConfiguracion`),
  KEY `fk_idrapidez_idx` (`idrapidez`),
  CONSTRAINT `fk_idrapidez` FOREIGN KEY (`idrapidez`) REFERENCES `rapidez` (`idrapidez`)
);



CREATE TABLE `resultado` (
  `idResultado` int NOT NULL AUTO_INCREMENT,
  `PH` float DEFAULT NULL,
  `Configuracion_idConfiguracion` int DEFAULT NULL,
  `Fecha` date DEFAULT NULL,
  PRIMARY KEY (`idResultado`),
  KEY `fk_Resultado_Configuracion_idx` (`Configuracion_idConfiguracion`),
  CONSTRAINT `fk_configuracion` FOREIGN KEY (`Configuracion_idConfiguracion`) REFERENCES `configuracion` (`idConfiguracion`)
);