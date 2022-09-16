import os
import sys
import xml.etree.ElementTree as ET

HARNESS_XML_PATH = r'%s\GRL\Thread1.2\Web\data\deviceInputFields.xml' % os.environ['systemdrive']


def main():
    tree = ET.parse(HARNESS_XML_PATH)
    root = tree.getroot()
    added = ET.parse(sys.argv[1]).getroot()

    added_names = set(device.attrib['name'] for device in added.iter('DEVICE'))
    # If some devices already exist, remove them first, and then add them back in case of update
    removed_devices = [device for device in root.iter('DEVICE') if device.attrib['name'] in added_names]

    for device in removed_devices:
        root.remove(device)
    for device in added.iter('DEVICE'):
        root.append(device)

    tree.write(HARNESS_XML_PATH)


if __name__ == '__main__':
    main()
