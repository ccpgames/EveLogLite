import ctypes
import logging
import os
import socket
import sys
import time

VERSION = 2
PORT = 0xcc9


class _ConnectionMessage(ctypes.Structure):
    _fields_ = [('version', ctypes.c_uint32), ('pid', ctypes.c_int64), ('machine_name', ctypes.c_char * 32),
                ('executable_path', ctypes.c_char * 260)]


class _TextMessage(ctypes.Structure):
    _fields_ = [('timestamp', ctypes.c_uint64), ('severity', ctypes.c_uint32), ('module', ctypes.c_char * 32),
                ('channel', ctypes.c_char * 32), ('message', ctypes.c_char * 256)]


class _TextOrConnection(ctypes.Union):
    _fields_ = [('connection', _ConnectionMessage), ('text', _TextMessage)]


class _Message(ctypes.Structure):
    _fields_ = [('type', ctypes.c_uint32), ('body', _TextOrConnection)]


class _MessageType(object):
    CONNECTION_MESSAGE = 0
    SIMPLE_MESSAGE = 1
    LARGE_MESSAGE = 2
    CONTINUATION_MESSAGE = 3
    CONTINUATION_END_MESSAGE = 4


class Severity(object):
    INFO = 0
    NOTICE = 1
    WARNING = 2
    ERROR = 3


class LogLiteClient(object):
    def __init__(self, server='127.0.0.1', pid=None, machine_name=None, executable_path=None):
        if pid is None:
            pid = os.getpid()
        if machine_name is None:
            machine_name = socket.gethostname()
        if executable_path is None:
            executable_path = sys.argv[0]

        self.socket = socket.create_connection((server, 0xcc9))

        msg = _Message()
        msg.type = _MessageType.CONNECTION_MESSAGE
        msg.body.connection.version = VERSION
        msg.body.connection.pid = pid
        msg.body.connection.machine_name = machine_name
        msg.body.connection.executable_path = executable_path
        self.socket.sendall(buffer(msg)[:])

    def log(self, severity, message, timestamp=None, module='', channel=''):
        msg = _Message()
        msg.body.text.timestamp = timestamp or int(time.time() * 1000)
        msg.body.text.severity = severity
        msg.body.text.module = module
        msg.body.text.channel = channel
        if len(message) < 255:
            msg.type = _MessageType.SIMPLE_MESSAGE
            msg.body.text.message = message
            self.socket.sendall(buffer(msg)[:])
        else:
            offset = 0
            msg.type = _MessageType.LARGE_MESSAGE
            while offset < len(message):
                msg.body.text.message = message[offset:offset + 255]
                self.socket.sendall(buffer(msg)[:])
                offset += 255
                if offset + 255 >= len(message):
                    msg.type = _MessageType.CONTINUATION_END_MESSAGE
                else:
                    msg.type = _MessageType.CONTINUATION_MESSAGE

LEVEL_MAP = {
    logging.CRITICAL:   Severity.ERROR,
    logging.ERROR:      Severity.ERROR,
    logging.WARNING:    Severity.WARNING,
    logging.INFO:       Severity.NOTICE,
    logging.DEBUG:      Severity.INFO,
    logging.NOTSET:     Severity.INFO}


class LogLiteHandler(logging.Handler):
    def __init__(self, client):
        super(LogLiteHandler, self).__init__()
        self.client = client

    def emit(self, record):
        # noinspection PyBroadException
        try:
            if '.' in record.name:
                channel, module = record.name.split('.', 1)
            else:
                channel, module = record.name, 'General'
            self.client.log(LEVEL_MAP.get(record.levelno, Severity.INFO), self.format(record), module=module,
                            channel=channel)
        except:
            self.handleError(record)
