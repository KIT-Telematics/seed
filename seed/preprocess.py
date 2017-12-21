"""preprocess bundle xml
"""

import re
import xml.etree.ElementTree as ET

def preprocess_bundle(bundlepath, destination, override_parameters):
    """preprocess bundle xml

    overrides predefined parameters, expands parameter expressions in
    element attribute values and saves bundle to destination
    Args:
        bundlepath: path to bundle
        destination: path the bundle destination
        override_parameters: parameters to override
    """
    tree = ET.parse(bundlepath)
    root = tree.getroot()
    __preprocess_params(root, override_parameters)
    __preprocess_groups(root, "node")
    __preprocess_groups(root, "interface")
    __preprocess_groups(root, "link")
    tree.write(destination)

def __preprocess_params(root, override_parameters):
    """preprocess parameters

    overrides predefined parameters and expands parameter expressions in
    element attribute values
    Args:
        bundlepath: path to bundle
        override_parameters: parameters to override
    """
    elem_parameters = root.find('parameters')
    root.remove(elem_parameters)
    parameters = {parameter.attrib['name']:parameter.attrib['value']
                  for parameter in elem_parameters.iter(tag='parameter')}
    for name, value in override_parameters.items():
        assert name in parameters
        parameters[name] = value
    regex = re.compile(r'(\$\([^)]*\))')
    for element in root.iter():
        for key in element.attrib.keys():
            element.attrib[key] = "".join(
                [s if i % 2 == 0 else parameters[s[2:-1]]
                 for i, s in enumerate(regex.split(element.attrib[key]))])

def __preprocess_groups(root, tag):
    """preprocess groups

    copies group attributes to elements
    Args:
        root: root element
        tag: tag of elements
    """
    groups = {}
    for group in root.find(f"./topology/{tag}groups"):
        name = group.attrib.pop('name')
        groups[name] = group.attrib
        group.attrib = {'name': name}

    for element in root.iter(tag=tag):
        element_groups = element.attrib.get("groups", None)
        if element_groups is None:
            continue
        attrib = {}
        for group in element_groups.split():
            attrib.update(groups[group])
        attrib.update(element.attrib)
        element.attrib = attrib
