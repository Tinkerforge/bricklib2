{0x0002, &meter_register_set.line_to_neutral_volts[0], 0.01, METER_REGISTER_DATA_TYPE_INT32, false},
{0x0004, &meter_register_set.line_to_neutral_volts[1], 0.01, METER_REGISTER_DATA_TYPE_INT32, false},
{0x0006, &meter_register_set.line_to_neutral_volts[2], 0.01, METER_REGISTER_DATA_TYPE_INT32, false},
{0x0008, &meter_register_set.current[0], 0.0001, METER_REGISTER_DATA_TYPE_INT32, true},
{0x000A, &meter_register_set.current[1], 0.0001, METER_REGISTER_DATA_TYPE_INT32, true},
{0x000C, &meter_register_set.current[2], 0.0001, METER_REGISTER_DATA_TYPE_INT32, true},
{0x000E, &meter_register_set.line1_to_line2_volts, 0.01, METER_REGISTER_DATA_TYPE_INT32, false},
{0x0010, &meter_register_set.line2_to_line3_volts, 0.01, METER_REGISTER_DATA_TYPE_INT32, false},
{0x0012, &meter_register_set.line3_to_line1_volts, 0.01, METER_REGISTER_DATA_TYPE_INT32, false},
{0x0014, &meter_register_set.power[0], 0.01, METER_REGISTER_DATA_TYPE_INT32, false},
{0x0016, &meter_register_set.power[1], 0.01, METER_REGISTER_DATA_TYPE_INT32, false},
{0x0018, &meter_register_set.power[2], 0.01, METER_REGISTER_DATA_TYPE_INT32, false},
{0x001A, &meter_register_set.volt_amps_reactive[0], 0.01, METER_REGISTER_DATA_TYPE_INT32, false},
{0x001C, &meter_register_set.volt_amps_reactive[1], 0.01, METER_REGISTER_DATA_TYPE_INT32, false},
{0x001E, &meter_register_set.volt_amps_reactive[2], 0.01, METER_REGISTER_DATA_TYPE_INT32, false},
{0x0020, &meter_register_set.volt_amps[0], 0.01, METER_REGISTER_DATA_TYPE_INT32, false},
{0x0022, &meter_register_set.volt_amps[1], 0.01, METER_REGISTER_DATA_TYPE_INT32, false},
{0x0024, &meter_register_set.volt_amps[2], 0.01, METER_REGISTER_DATA_TYPE_INT32, false},
{0x0026, &meter_register_set.power_factor[0], 0.0001, METER_REGISTER_DATA_TYPE_INT32, false},
{0x0028, &meter_register_set.power_factor[1], 0.0001, METER_REGISTER_DATA_TYPE_INT32, false},
{0x002A, &meter_register_set.power_factor[2], 0.0001, METER_REGISTER_DATA_TYPE_INT32, false},
{0x0032, &meter_register_set.frequency_of_supply_voltages, 0.01, METER_REGISTER_DATA_TYPE_INT32, false},
{0x003A, &meter_register_set.total_system_power, 0.01, METER_REGISTER_DATA_TYPE_INT32, true},
{0x0040, &meter_register_set.total_system_power_factor, 0.0001, METER_REGISTER_DATA_TYPE_INT32, false},
{0x1A20, &meter_register_set.total_import_kwh, 0.001, METER_REGISTER_DATA_TYPE_INT32, false},
{0x1A22, &meter_register_set.total_export_kwh, 0.001, METER_REGISTER_DATA_TYPE_INT32, false},
{0x1A24, &meter_register_set.total_import_kvarh, 0.001, METER_REGISTER_DATA_TYPE_INT32, false},
{0x1A26, &meter_register_set.total_export_kvarh, 0.001, METER_REGISTER_DATA_TYPE_INT32, false},
{0x1A34, &meter_register_set.import_kwh[0], 0.001, METER_REGISTER_DATA_TYPE_INT32, false},
{0x1A36, &meter_register_set.export_kwh[0], 0.001, METER_REGISTER_DATA_TYPE_INT32, false},
{0x1A38, &meter_register_set.import_kvarh[0], 0.001, METER_REGISTER_DATA_TYPE_INT32, false},
{0x1A3A, &meter_register_set.export_kvarh[0], 0.001, METER_REGISTER_DATA_TYPE_INT32, false},
{0x1A48, &meter_register_set.import_kwh[1], 0.001, METER_REGISTER_DATA_TYPE_INT32, false},
{0x1A4A, &meter_register_set.export_kwh[1], 0.001, METER_REGISTER_DATA_TYPE_INT32, false},
{0x1A4C, &meter_register_set.import_kvarh[1], 0.001, METER_REGISTER_DATA_TYPE_INT32, false},
{0x1A4E, &meter_register_set.export_kvarh[1], 0.001, METER_REGISTER_DATA_TYPE_INT32, false},
{0x1A5C, &meter_register_set.import_kwh[2], 0.001, METER_REGISTER_DATA_TYPE_INT32, false},
{0x1A5E, &meter_register_set.export_kwh[2], 0.001, METER_REGISTER_DATA_TYPE_INT32, false},
{0x1A60, &meter_register_set.import_kvarh[2], 0.001, METER_REGISTER_DATA_TYPE_INT32, false},
{0x1A62, &meter_register_set.export_kvarh[2], 0.001, METER_REGISTER_DATA_TYPE_INT32, false},
{0, NULL, 0.0, 0, false},
