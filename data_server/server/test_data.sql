INSERT INTO ShutEyeDeviceEnergyDataHistorical (appliance_name, local_time, today_runtime, month_runtime, today_energy, month_energy)
VALUES 
('microwave', '2025-01-01 08:00:00', 120, 1500, 300, 4500),
('microwave', '2025-01-02 09:00:00', 100, 1600, 250, 4600),
('blender', '2025-01-01 10:00:00', 90, 1300, 200, 4000),
('toaster', '2025-01-01 11:00:00', 130, 1400, 280, 4200);

INSERT INTO ShutEyeDeviceEnergyDataPeriodicMeasurement (appliance_name, local_time, current_power, distance_ultrasonic, distance_bluetooth, distance_ultrawideband, user_presence_detected)
VALUES 
('microwave', '2025-01-01 08:00:00', 150, 20, 30, 50, TRUE),
('blender', '2025-01-01 09:00:00', 120, 25, 35, 45, FALSE),
('toaster', '2025-01-01 10:00:00', 180, 15, 25, 40, TRUE);


SELECT * FROM ShutEyeDeviceEnergyDataHistorical;
SELECT * FROM ShutEyeDeviceEnergyDataPeriodicMeasurement;


DELETE FROM ShutEyeDeviceEnergyDataHistorical WHERE appliance_name = 'microwave';
DELETE FROM ShutEyeDeviceEnergyDataPeriodicMeasurement WHERE appliance_name = 'microwave';

DELETE FROM ShutEyeDeviceEnergyDataHistorical WHERE appliance_name = 'blender';
DELETE FROM ShutEyeDeviceEnergyDataPeriodicMeasurement WHERE appliance_name = 'blender';

DELETE FROM ShutEyeDeviceEnergyDataHistorical WHERE appliance_name = 'toaster';
DELETE FROM ShutEyeDeviceEnergyDataPeriodicMeasurement WHERE appliance_name = 'toaster';