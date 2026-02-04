import os
from esphome import core # type: ignore 
import esphome.codegen as cg # type: ignore
import esphome.config_validation as cv # type: ignore
from esphome.components import uart # type: ignore
from esphome.const import CONF_ID # type: ignore

DEPENDENCIES = ['uart']
AUTO_LOAD = ['uart']

notecard_ns = cg.esphome_ns.namespace('notecard')
Notecard = notecard_ns.class_('Notecard', cg.Component, uart.UARTDevice)

CONF_NOTECARD_ID = 'notecard_id'
CONF_PROJECT_ID = 'project_id'
CONF_ORG = 'org'
CONF_SYNC_INTERVAL = 'sync_interval'

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(Notecard),
    cv.Required(CONF_PROJECT_ID): cv.string,
    cv.Optional(CONF_ORG): cv.string,
    cv.Optional(CONF_SYNC_INTERVAL, default='4h'): cv.positive_time_period_seconds,
}).extend(uart.UART_DEVICE_SCHEMA).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    cg.add(var.set_project_id(config[CONF_PROJECT_ID]))
    
    if CONF_ORG in config:
        cg.add(var.set_org(config[CONF_ORG]))
    
    cg.add(var.set_sync_interval(config[CONF_SYNC_INTERVAL]))