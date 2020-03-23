"""
* Copyright 2019 NXP
* All rights reserved.
*
* SPDX-License-Identifier: BSD-3-Clause
"""

#!python

from collections import namedtuple
import re
import argparse
import subprocess
import struct
from Crypto.Signature import pkcs1_15
from Crypto.PublicKey import RSA
from Crypto.Hash import SHA256
from Crypto.Util import number
import binascii
import os

parser = argparse.ArgumentParser(description='JN518x Image Header Generator')
parser.add_argument('in_file', help="Binary to be post-processed: generating header and optionally appending certificate and/or signature.")
parser.add_argument('out_file', nargs='?')
parser.add_argument('-g', '--signature_path', help="Sets directory from which certificate and private key are to be retrieved")
parser.add_argument('-k', '--key', action='store_true', help="2048 bits RSA private key in PEM format used to sign the full image. If -c option is used the full image includes the certificate + the signature of the certificate. The key shall be located in the same directory as the image_tool script. See priv_key.pem example.")
parser.add_argument('-p', '--password',help="This is the pass phrase from which the encryption key is derived. This parameter is only required if key provided through the -k option is a PEM encrypted key.")
parser.add_argument('-c', '--certificate', action='store_true', help="When option is selected, the certificate cert.bin is appended to the image.")
parser.add_argument('-a', '--appcore_image', action='store_true',help="This parameter is only relevant if dual application (app1 image) shall reside in flash. Do not use in conjunction with -i option.")
parser.add_argument('-i', '--image_identifier', type=int, default=0, help="This parameter is to set the archive identifier. 0: SSBL or legacy JN518x/QN9090 applications, loaded at 0x00000000. 1: ZigBee  application loaded at address 0x00004000 by default. 2: BLE image loaded at address 0x00054000 by default")
parser.add_argument('-t', '--target_addr', type=int, help="Target address of image. Used in conjucntion with -i option to override the default set by image identifier, or with -a option to specify address of the appcore image (app1 image).")
parser.add_argument('-r', '--rev', type=int, default=2, help="This is the revision of the JN5189 chip. Set 1 for ES1 and 2 for ES2. Default is ES2.")
parser.add_argument('-s', '--stated_size', type=int, default=0x48000, help="This is the stated size of the image in bytes. Default is 0x48000.")
parser.add_argument('-v', '--version', type=int, default=0, help="Image version. Default is 0.")
parser.add_argument('-b', '--verbose', type=int, default=0, help="verbosity level. Default is 0.")

args = parser.parse_args()

elf_file_name = args.in_file
bin_file_name = elf_file_name.split(".")[0]+'_temp.bin'

if args.out_file is None:
    args.out_file = elf_file_name

verbose = args.verbose != 0



def parse_sections(file):
    sections = {}

    Section = namedtuple('Section', ['idx', 'name', 'size', 'vma', 'lma', 'offset', 'align', 'flags'])

    objdump = subprocess.check_output(['arm-none-eabi-objdump', '-h', file])

    section_re = re.compile(r'(?P<idx>\d*)\s'
                            r'(?P<name>[.\w]*)\s*'
                            r'(?P<size>[0-9a-f]{8})\s*'
                            r'(?P<vma>[0-9a-f]{8})\s*'
                            r'(?P<lma>[0-9a-f]{8})\s*'
                            r'(?P<offset>[0-9a-f]{8})\s*'
                            r'(?P<align>[0-9*]*)\s*'
                            r'(?P<flags>(?:[ \t]*[\w,]*)*)')

    for match in re.finditer(section_re, objdump):
        sec_dict = match.groupdict()

        sec_dict['idx'] = int(sec_dict['idx'])

        for attr in ['vma', 'lma', 'size', 'offset']:
            sec_dict[attr] = int(sec_dict[attr], 16)

        sec_dict['align'] = eval(sec_dict['align'])

        sections[sec_dict['name']] = Section(**sec_dict)

    return sections


#
# JN518x ES1 version
######################
if args.rev == 1:

    BOOT_BLOCK_MARKER = 0xBB0110BB

    header_struct     = struct.Struct('<7LLLLL')
    boot_block_struct = struct.Struct('<6LQ')

    sections = parse_sections(args.in_file)

    last_section = None

    for name, section in sections.iteritems():
        if 'LOAD' in section.flags:
            if last_section is None or section.lma > last_section.lma:
                if section.size > 0:
                    last_section = section

    if args.appcore_image is True:
        image_size = last_section.lma + last_section.size - args.target_appcore_addr
    else:
        image_size = last_section.lma + last_section.size

    dump_section = subprocess.check_output(['arm-none-eabi-objcopy', '--dump-section', '%s=data.bin' % last_section.name, args.in_file])

    if args.appcore_image is True:
        boot_block = boot_block_struct.pack(BOOT_BLOCK_MARKER, 1, args.target_appcore_addr, image_size + boot_block_struct.size, 0, 0, 0)
    else:
        boot_block = boot_block_struct.pack(BOOT_BLOCK_MARKER, 0, 0, image_size + boot_block_struct.size, 0, 0, 0)

    with open('data.bin', 'ab') as out_file:
        out_file.write(boot_block)

    update_section = subprocess.check_output(['arm-none-eabi-objcopy', '--update-section', '%s=data.bin' % last_section.name, args.in_file, args.out_file])

    first_section = None

    for name, section in sections.iteritems():
        if 'LOAD' in section.flags:
            if first_section is None or section.lma < first_section.lma:
                first_section = section

    with open(args.out_file, 'r+b') as elf_file:
        elf_file.seek(first_section.offset)
        vectors = elf_file.read(header_struct.size)

        fields = list(header_struct.unpack(vectors))

        vectsum = 0

        for x in range(7):
            vectsum += fields[x]

        fields[7]  = (~vectsum & 0xFFFFFFFF) + 1
        if args.appcore_image is True:
            fields[9]  = 0x02794498
        else:
            fields[9]  = 0x98447902
        #fields[9]  = 0x98447902
        fields[10] = image_size

        print "Writing checksum {:08x} to file {:s}".format(vectsum, args.out_file)

        elf_file.seek(first_section.offset)
        elf_file.write(header_struct.pack(*fields))

#
# JN518x ES2 version
######################
else:
    is_signature = False
    if args.signature_path is not None:
        sign_dir_path = os.path.join(os.path.dirname(__file__), args.signature_path)
        priv_key_file_path = os.path.join(sign_dir_path, 'priv_key.pem')
        cert_file_path = os.path.join(sign_dir_path, 'cert.bin')
    else:
        sign_dir_path = os.path.join(os.path.dirname(__file__), '')
        priv_key_file_path = os.path.join(sign_dir_path, 'testkey_es2.pem')
        cert_file_path = os.path.join(sign_dir_path, 'certif_es2')

    if args.key is True:
        key_file_path = priv_key_file_path
        if verbose:
            print "key path is " + key_file_path
        if (os.path.isfile(key_file_path)):
            key_file=open(key_file_path, 'r')
            key = RSA.importKey(key_file.read(), args.password)
            print "Private RSA key processing..."
            is_signature = True

    bin_output = subprocess.check_output(['arm-none-eabi-objcopy', '-O', 'binary', elf_file_name, bin_file_name])

    with open(bin_file_name, 'rb') as in_file:
        input_file = in_file.read()

    BOOT_BLOCK_MARKER      = 0xBB0110BB
    IMAGE_HEADER_MARKER    = 0x98447902
    IMAGE_HEADER_APP_CORE  = 0x02794498
    IMAGE_HEADER_ESCORE    = IMAGE_HEADER_MARKER
    SSBL_OR_LEGACY_ADDRESS = 0x00000000
    SSBL_STATED_SIZE       = 0x3000
    ZB_TARGET_ADDRESS      = SSBL_STATED_SIZE * 2
    ZB_STATED_SIZE         = 0x4f000
    BLE_TARGET_ADDRESS     = ZB_TARGET_ADDRESS + ZB_STATED_SIZE
    BLE_STATED_SIZE        = 0x3b000

    header_struct     = struct.Struct('<7LLLLL')
    boot_block_struct = struct.Struct('<8L')

    boot_block_marker = BOOT_BLOCK_MARKER
    if args.image_identifier is not None:
        image_iden = args.image_identifier
    else:
        image_iden = 0

    # Set header marker and image address based on image identifier (-i option)
    if verbose:
        print "Image Identifier is {:d}".format(image_iden)
    if image_iden == 0:
        img_header_marker = IMAGE_HEADER_MARKER + image_iden
        image_addr = SSBL_OR_LEGACY_ADDRESS
        stated_size = SSBL_STATED_SIZE
    elif image_iden == 1:
        img_header_marker = IMAGE_HEADER_MARKER + image_iden
        image_addr = ZB_TARGET_ADDRESS
        stated_size = ZB_STATED_SIZE
    elif image_iden == 2:
        img_header_marker = IMAGE_HEADER_MARKER + image_iden
        image_addr = BLE_TARGET_ADDRESS
        stated_size = BLE_STATED_SIZE
    else:
        image_addr = 0
        stated_size = 0

    # Overwrite defaults for image address, stated size and header marker (-t, -s, -a options)
    if args.target_addr is not None:
        image_addr = args.target_addr

    if args.stated_size is not None:
        stated_size = args.stated_size

    if args.appcore_image is True:
        img_header_marker  = IMAGE_HEADER_APP_CORE

    if verbose:
        print "image_iden=%d image_addr=%x" % (image_iden, image_addr)
        print "boot_block_marker=%x" % (boot_block_marker)

    sections = parse_sections(elf_file_name)

    last_section = None
    for name, section in sections.iteritems():
        if 'LOAD' in section.flags:
            if last_section is None or section.lma > last_section.lma:
                if section.size > 0:
                    last_section = section

    image_size = last_section.lma + last_section.size - image_addr
    if verbose:
        print "Last Section LMA={:08x} Size={:08x}".format(last_section.lma, last_section.size)
        print "ImageAddress={:08x}".format(image_addr)

    first_section = None

    for name, section in sections.iteritems():
        # print "Section: {:s} {:s} {:x} {:x}".format(name, section.flags, section.lma, section.size)
        if 'LOAD' in section.flags:
            if first_section is None or section.lma < first_section.lma:
                first_section = section

    header=""
    with open(args.out_file, 'r+b') as elf_file:
        elf_file.seek(first_section.offset)
        vectors = elf_file.read(header_struct.size)

        fields = list(header_struct.unpack(vectors))

        vectsum = 0
        for x in range(7):
            vectsum += fields[x]

        fields[7]  = (~vectsum & 0xFFFFFFFF) + 1
        fields[8]  = img_header_marker
        fields[9]  = image_size                    #offset of boot block structure

        #Compute crc
        head_struct     = struct.Struct('<10L')
        if verbose:
            for i in range(10):
                print "Header[{:d}]= {:08x}".format(i, fields[i])
        values = head_struct.pack(fields[0],
                                  fields[1],
                                  fields[2],
                                  fields[3],
                                  fields[4],
                                  fields[5],
                                  fields[6],
                                  fields[7],
                                  fields[8],
                                  fields[9])
        fields[10] = binascii.crc32(values) & 0xFFFFFFFF

        print "Writing checksum {:08x} to file {:s}".format(vectsum, args.out_file)
        print "Writing CRC32 of header {:08x} to file {:s}".format(fields[10], args.out_file)


        elf_file.seek(first_section.offset)
        header = header_struct.pack(*fields);
        elf_file.write(header)

    dump_section = subprocess.check_output(['arm-none-eabi-objcopy',
                                            '--dump-section',
                                            '%s=data.bin' % last_section.name,
                                            args.out_file])

    certificate = ""
    certificate_offset = 0
    signature = ""

    if (args.certificate is True):
        certificate_offset = image_size + boot_block_struct.size
        certif_file_path = cert_file_path
        if verbose:
            print "Cert key path is " + cert_file_path
        if (os.path.isfile(certif_file_path)):
            certif_file=open(certif_file_path, 'rb')
            certificate = certif_file.read()

        print "Certificate processing..."
        if len(certificate) != (40+256+256):
            print "Certificate error"

    if verbose:
        print "stated size is {:08x}".format(stated_size)

    if args.appcore_image is True:
        boot_block_id = 1
    else:
        boot_block_id = 0

    boot_block = boot_block_struct.pack(boot_block_marker,
                                        boot_block_id,
                                        image_addr,
                                        image_size + boot_block_struct.size + len(certificate),
                                        stated_size,
                                        certificate_offset, 0, args.version)

    if (is_signature == True):
        # Sign the complete image
        message = header + input_file[header_struct.size:] + boot_block + certificate
        hash = SHA256.new(message)

        out_file_path = os.path.join(os.path.dirname(__file__), 'dump_python.bin')
        file_out=open(out_file_path, 'wb')
        file_out.write(message)

        signer = pkcs1_15.new(key)
        signature = signer.sign(hash)

        print "Signature processing..."


    with open('data.bin', 'ab') as out_file:
        out_file.write(boot_block+certificate+signature)

    update_section = subprocess.check_output(['arm-none-eabi-objcopy',
                                              '--update-section',
                                              '%s=data.bin' % last_section.name,
                                              args.out_file,
                                              args.out_file])

    os.remove(bin_file_name)
