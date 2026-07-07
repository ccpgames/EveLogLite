#!/usr/bin/env python3

import argparse
import sys
from . import LogLiteClient, Severity


def main():
    parser = argparse.ArgumentParser(
        description='Flood a LogLite server with messages carrying monotonic sequence numbers.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
  python3 -m clients.python.flood_helper --count 10
  python3 -m clients.python.flood_helper --count 200000 --host 127.0.0.1 --port 3273 --severity notice
        '''
    )

    parser.add_argument(
        '--count', '-c',
        type=int,
        default=10,
        help='Number of messages to send (default: 10)'
    )
    parser.add_argument(
        '--host',
        default='127.0.0.1',
        help='LogLite server host (default: 127.0.0.1)'
    )
    parser.add_argument(
        '--port',
        type=int,
        default=3273,
        help='LogLite server port (default: 3273, 0xCC9 in hex)'
    )
    parser.add_argument(
        '--severity',
        choices=['info', 'notice', 'warning', 'error'],
        default='notice',
        help='Message severity level (default: notice)'
    )

    args = parser.parse_args()

    # Map severity string to constant
    severity_map = {
        'info': Severity.INFO,
        'notice': Severity.NOTICE,
        'warning': Severity.WARNING,
        'error': Severity.ERROR,
    }

    try:
        # Create client and send CONNECTION_MESSAGE handshake
        client = LogLiteClient(server=args.host, pid=None, machine_name=None, executable_path=None)

        # Send configured count of messages with monotonic sequence numbers
        for seq in range(args.count):
            message_text = 'msg {}'.format(seq)
            client.log(
                severity=severity_map[args.severity],
                message=message_text,
                module='flood_helper',
                channel='test'
            )

        return 0

    except Exception as e:
        print('Error: {}'.format(e), file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
