CREATE DATABASE IF NOT EXISTS ShutEyeDataServer;
USE ShutEyeDataServer;

DROP TABLE IF EXISTS ShutEyeDeviceEnergyDataHistorical;

CREATE TABLE ShutEyeDeviceEnergyDataHistorical (
    appliance_name     VARCHAR(50) NOT NULL,
    local_time         DATETIME NOT NULL,
    today_runtime      INTEGER NOT NULL,
    month_runtime      INTEGER NOT NULL,
    today_energy       INTEGER NOT NULL,
    month_energy       INTEGER NOT NULL,
    PRIMARY KEY (appliance_name, local_time)
);

DROP TABLE IF EXISTS ShutEyeDeviceEnergyDataPeriodicMeasurement;

CREATE TABLE ShutEyeDeviceEnergyDataPeriodicMeasurement (
    appliance_name           VARCHAR(50) NOT NULL,
    local_time               DATETIME NOT NULL,
    current_power            INTEGER NOT NULL,
    distance_ultrasonic      INTEGER NOT NULL,
    distance_bluetooth       INTEGER NOT NULL,
    distance_ultrawideband   INTEGER NOT NULL,
    user_presence_detected   BOOLEAN NOT NULL,
    PRIMARY KEY (appliance_name, local_time)
);
