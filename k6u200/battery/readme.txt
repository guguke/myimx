替换原来的内核/driver/input中的max11801_ts.c，重新编译
cat sys/class/power_supply/max8903-charger/uevent
可以查看电压，电量百分比，充电状态

POWER_SUPPLY_VOLTAGE_NOW	电压值
POWER_SUPPLY_STATUS		充电状态（Discharging，Charging，FULL）
POWER_SUPPLY_PRESENT=1
POWER_SUPPLY_CAPACITY=100	电量百分比
POWER_SUPPLY_VOLTAGE_MAX_DESIGN=4200000	设计最大电压
POWER_SUPPLY_VOLTAGE_MIN_DESIGN=2800000	设计最小电压
POWER_SUPPLY_HEALTH=Good
POWER_SUPPLY_CAPACITY_LEVEL=Normal


max11801_ts.c
sample_data = 44*total/273;
是修改分压的