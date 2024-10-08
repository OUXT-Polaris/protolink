import importlib
import sys
from rosidl_parser.definition import NamespacedType
import re
from jinja2 import Environment, FileSystemLoader
import os


def is_sequence_type(ros2_message_field_type):
    pattern = r"sequence<([^>]+)>"
    match = re.search(pattern, ros2_message_field_type)
    if match:
        return match.group(1)
    return None


def get_message_fields(msg_type_name):
    try:
        msg_module, msg_class = msg_type_name.split("/")
        module = importlib.import_module(f"{msg_module}.msg")
        msg_class = getattr(module, msg_class)
    except (ImportError, AttributeError) as e:
        print(f"Error: Could not import message type {msg_type_name}: {e}")
        return None

    return msg_class.get_fields_and_field_types()


def to_proto_type(ros2_message_field_type):
    if (
        ros2_message_field_type == "uint8"
        or ros2_message_field_type == "uint16"
        or ros2_message_field_type == "uint32"
    ):
        return "uint32"
    elif (
        ros2_message_field_type == "int8"
        or ros2_message_field_type == "int16"
        or ros2_message_field_type == "int32"
    ):
        return "int32"
    elif (
        ros2_message_field_type == "int64"
        or ros2_message_field_type == "uint64"
        or ros2_message_field_type == "string"
    ):
        return ros2_message_field_type
    elif ros2_message_field_type == "float32":
        return "float"
    elif ros2_message_field_type == "float64" or ros2_message_field_type == "double":
        return "double"
    elif is_sequence_type(ros2_message_field_type) != None:
        return "repeated " + to_proto_type(is_sequence_type(ros2_message_field_type))
    elif "/" in ros2_message_field_type:
        if get_message_fields(ros2_message_field_type) != None:
            return ros2_message_field_type
        raise Exception("Failed to get data infomation, please check ros2 prefix path.")
    raise Exception("Unspoorted built-in type : " + ros2_message_field_type)


def append_conversions_for_template(namespace, field_type, conversions):
    if "/" in field_type:
        if namespace != "":
            builtin_types = []
            user_types = []
            fields = get_message_fields(field_type)
            for field_name_in_child, field_type_in_child in fields.items():
                if "/" in field_type_in_child:
                    user_types.append(field_name_in_child)
                    append_conversions_for_template(
                        namespace
                        + "::"
                        + field_type.split("/")[0]
                        + "__"
                        + field_type.split("/")[1],
                        field_type_in_child,
                        conversions,
                    )
                else:
                    builtin_types.append(field_name_in_child)
            conversions.append(
                {
                    "proto": namespace
                    + "::"
                    + field_type.split("/")[0]
                    + "__"
                    + field_type.split("/")[1],
                    "ros2": field_type.split("/")[0]
                    + "::msg::"
                    + field_type.split("/")[1],
                    "members": {
                        "builtin_types": builtin_types,
                        "user_types": user_types,
                    },
                }
            )
        else:
            builtin_types = []
            user_types = []
            fields = get_message_fields(field_type)
            for field_name_in_child, field_type_in_child in fields.items():
                if "/" in field_type_in_child:
                    user_types.append(field_name_in_child)
                else:
                    builtin_types.append(field_name_in_child)
            conversions.append(
                {
                    "proto": "protolink__"
                    + field_type.split("/")[0]
                    + "__"
                    + field_type.split("/")[1]
                    + "::"
                    + field_type.split("/")[0]
                    + "__"
                    + field_type.split("/")[1],
                    "ros2": field_type.split("/")[0]
                    + "::msg::"
                    + field_type.split("/")[1],
                    "members": {
                        "builtin_types": builtin_types,
                        "user_types": user_types,
                    },
                }
            )
        return conversions
    else:
        return conversions


def to_proto_message_definition(field_type, field_name, message_index):
    if "/" in field_type:
        fields = get_message_fields(field_type)
        base_proto_string = (
            "message "
            + field_type.split("/")[0]
            + "__"
            + field_type.split("/")[1]
            + " {\n"
        )

        message_index_in_child = 1

        for field_name_in_child, field_type_in_child in fields.items():
            base_proto_string = base_proto_string + to_proto_message_definition(
                field_type_in_child, field_name_in_child, message_index_in_child
            )

            message_index_in_child = message_index_in_child + 1
        base_proto_string = base_proto_string + "}\n"

        return (
            base_proto_string
            + "\n"
            + field_type.split("/")[0]
            + "__"
            + field_type.split("/")[1]
            + " "
            + field_name
            + " = "
            + str(message_index)
            + ";\n"
        )
    else:
        return (
            to_proto_type(field_type)
            + " "
            + field_name
            + " = "
            + str(message_index)
            + ";\n"
        )


def get_message_structure(msg_type_name, output_file, header_file, source_file):
    env = Environment(
        loader=FileSystemLoader(searchpath=os.path.dirname(os.path.abspath(__file__)))
    )
    template_header = env.get_template("template_converter.hpp.jinja")
    template_cpp = env.get_template("template_converter.cpp.jinja")
    conversions = append_conversions_for_template("", msg_type_name, [])

    ros2_message_header = ""
    for splited in re.split(r"(?=[A-Z])", msg_type_name.split("/")[1]):
        if ros2_message_header == "":
            ros2_message_header = splited.lower()
        else:
            ros2_message_header = ros2_message_header + "_" + splited.lower()

    data = {
        "include_guard": "CONVERSION_"
        + msg_type_name.split("/")[0].upper()
        + "__"
        + msg_type_name.split("/")[1].upper()
        + "_HPP",
        "conversions": conversions,
        "ros2_header": msg_type_name.split("/")[0]
        + "/msg/"
        + ros2_message_header
        + ".hpp",
        "proto_header": msg_type_name.split("/")[0]
        + "__"
        + msg_type_name.split("/")[1]
        + ".pb.h",
        "conversion_header": header_file,
    }

    fields = get_message_fields(msg_type_name)

    print(f"Message: {msg_type_name}")
    print("Fields:")

    proto_string = 'syntax = "proto3";\n'
    proto_string = (
        proto_string
        + "package protolink__"
        + msg_type_name.split("/")[0]
        + "__"
        + msg_type_name.split("/")[1]
        + ";\n"
    )
    proto_string = (
        proto_string
        + "\nmessage "
        + msg_type_name.split("/")[0]
        + "__"
        + msg_type_name.split("/")[1]
        + " {\n"
    )

    message_index = 1

    for field_name, field_type in fields.items():
        print(f"  - {field_name}: {field_type} -> {to_proto_type(field_type)}")
        conversions = append_conversions_for_template(
            "protolink__"
            + msg_type_name.split("/")[0]
            + "__"
            + msg_type_name.split("/")[1]
            + "::"
            + msg_type_name.split("/")[0]
            + "__"
            + msg_type_name.split("/")[1],
            field_type,
            conversions,
        )
        proto_string = proto_string + to_proto_message_definition(
            field_type, field_name, message_index
        )
        message_index = message_index + 1
    proto_string = proto_string + "}"

    print("\nProto file => \n")
    print(proto_string)
    with open(output_file, mode="w") as f:
        f.write(proto_string)

    with open(header_file, "w") as f:
        f.write(template_header.render(data))
    with open(source_file, "w") as f:
        f.write(template_cpp.render(data))


if __name__ == "__main__":
    if len(sys.argv) != 5:
        print(
            "Usage: python3 generate_proto.py <message_type> <proto file> <conversion header file> <conversion source file>"
        )
        print(
            "Example: python3 generate_proto.py std_msgs/String std_msgs__String.proto conversion_std_msgs__String.hpp conversion_std_msgs__String.cpp"
        )
    else:
        msg_type_name = sys.argv[1]
        output_file = sys.argv[2]
        header_file = sys.argv[3]
        source_file = sys.argv[4]
        get_message_structure(msg_type_name, output_file, header_file, source_file)
