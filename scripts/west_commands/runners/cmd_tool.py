# Copyright (c) 2026, Telink Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

import os
import subprocess
import time

from runners.core import BuildConfiguration, RunnerCaps, ZephyrBinaryRunner


class CMDBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for TC.'''

    def __init__(self, cfg, cmd_path, address, erase=False):
        super().__init__(cfg)
        self.cmd_path = cmd_path
        self.address = address
        self.erase = bool(erase)

    @classmethod
    def name(cls):
        return 'cmd_tool'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'}, erase=True)

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--cmd-path', default='', help='path to TC installation root')
        parser.add_argument('--address', default='0x0', help='start flash address to write')

    @classmethod
    def do_create(cls, cfg, args):
        if args.cmd_path:
            cmd_path = args.cmd_path
        else:
            cmd_path = os.getenv('TELINK_CMD_BASE_DIR')
        return CMDBinaryRunner(cfg, cmd_path, args.address, args.erase)

    def do_run(self, command, **kwargs):
        self.require(self.cmd_path + '/TC')
        if command == "flash":
            self._flash()
        else:
            self.logger.error(f'{command} not supported!')

    def _flash(self):
        # obtain build configuration
        build_conf = BuildConfiguration(self.cfg.build_dir)
        # get chip
        soc_type = None
        if build_conf['CONFIG_SOC_RISCV_TELINK_TL321X']:
            soc_type = 'TL321X'
        if build_conf['CONFIG_SOC_RISCV_TELINK_TL322X']:
            soc_type = 'TL322X'
        if build_conf['CONFIG_SOC_RISCV_TELINK_TL323X']:
            soc_type = 'TL323X'
        if build_conf['CONFIG_SOC_RISCV_TELINK_TL521X']:
            soc_type = 'TL521X'
        if build_conf['CONFIG_SOC_RISCV_TELINK_TL721X']:
            soc_type = 'TL721X'
        if soc_type is None:
            print('only Telink chips are supported!')
            exit()
        # get flash size
        flash_size = str(build_conf['CONFIG_FLASH_SIZE']) + 'K'
        # get binary file
        bin_file = os.path.abspath(self.cfg.bin_file)
        # adjust flash offset for MCU boot application
        if (
            'CONFIG_BOOTLOADER_MCUBOOT' in build_conf
            and self.address == '0x0'
            and build_conf['CONFIG_BOOTLOADER_MCUBOOT']
        ):
            self.address = hex(build_conf['CONFIG_FLASH_LOAD_OFFSET'])
        # select chip
        print(f'select chip {soc_type}')
        if self._shell_execute(f'./TC setchip {soc_type}', self.cmd_path):
            print('failed!')
            exit()
        # activate
        print('activating...')
        if self._shell_execute('./TC ac', self.cmd_path):
            print('failed!')
            exit()
        # unlock flash
        print('unlocking flash...')
        if self._shell_execute('./TC ulf 0 0', self.cmd_path):
            print('failed!')
            exit()
        # erase flash
        if self.erase:
            print(f'erasing {flash_size}...')
            if self._shell_execute(f'./TC ef 0 {flash_size}', self.cmd_path):
                print('failed!')
                exit()
        # flash
        f_name = os.path.basename(bin_file)
        print(f'flashing "{f_name}" at offset {self.address}...')
        if self._shell_execute(f'./TC wf {self.address} -i "{bin_file}"', self.cmd_path):
            print('failed!')
            exit()
        print('resetting...')
        if self._shell_execute('./TC rst -f', self.cmd_path):
            print('failed!')
            exit()
        print('done!')

    @classmethod
    def _shell_execute(cls, command: str, path: str = None) -> int:
        if path is None:
            path = os.getcwd()
        print(command)
        exec_cmd = subprocess.Popen(
            command, shell=True, cwd=path, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
        )
        time.sleep(1e-2)
        progress_shown = False
        while True:
            ret_code = exec_cmd.poll()
            if ret_code is not None:
                if progress_shown:
                    print('')
                break
            progress_shown = True
            print('.', end='', flush=True)
            time.sleep(1)
        while ret_code:
            err = exec_cmd.stderr.read()
            if err != '':
                print(err, end='', flush=True)
                break
            out = exec_cmd.stdout.read()
            if out != '':
                print(out, end='', flush=True)
                break
            print('error!', end='', flush=True)
            break
        return ret_code
