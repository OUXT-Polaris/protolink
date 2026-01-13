import importlib
import os
import re
import sys

from jinja2 import Environment, FileSystemLoader
from rosidl_parser.definition import NamespacedType


def is_sequence_type(ros2_message_field_type: str) -> str | None:
    pattern = r"sequence<([^>]+)>"
    match = re.search(pattern, ros2_message_field_type)
    if match:
        return match.group(1)
    return None


def is_array_type(ros2_message_field_type: str) -> bool:
    pattern = r"\w*\[\d*\]$"
    match = re.search(pattern, ros2_message_field_type)
    return True if match else False


def array_to_proto_type(ros2_message_field_type: str) -> str:
    splited_field = re.split("[\[\]]", ros2_message_field_type)
    return splited_field[0]


def get_message_fields(msg_type_name: str) -> dict[str, str] | None:
    try:
        msg_module, msg_class = msg_type_name.split("/")
        module = importlib.import_module(f"{msg_module}.msg")
        msg_class = getattr(module, msg_class)
    except (ImportError, AttributeError) as e:
        print(f"Error: Could not import message type {msg_type_name}: {e}")
        return None

    return msg_class.get_fields_and_field_types()


def to_proto_type(ros2_message_field_type: str) -> str | None:
    if (
        ros2_message_field_type == "uint8"
        or ros2_message_field_type == "uint16"
        or ros2_message_field_type == "uint32"
    ):
        return "optional uint32"
    elif (
        ros2_message_field_type == "int8"
        or ros2_message_field_type == "int16"
        or ros2_message_field_type == "int32"
    ):
        return "optional int32"
    elif (
        ros2_message_field_type == "int64"
        or ros2_message_field_type == "uint64"
        or ros2_message_field_type == "string"
    ):
        return "optional " + ros2_message_field_type
    elif ros2_message_field_type == "float32" or ros2_message_field_type == "float":
        return "optional float"
    elif ros2_message_field_type == "float64" or ros2_message_field_type == "double":
        return "optional double"
    elif ros2_message_field_type == "boolean":
        return "optional bool"
    elif is_array_type(ros2_message_field_type) != None:
        return "repeated " + array_to_proto_type(ros2_message_field_type)
    elif "/" in ros2_message_field_type:
        if get_message_fields(ros2_message_field_type) != None:
            return ros2_message_field_type
        raise Exception("Failed to get data infomation, please check ros2 prefix path.")
    raise Exception("Unspoorted built-in type : " + ros2_message_field_type)


def append_conversions_for_template(
    namespace: str, field_type: str, conversions: list[dict], processed_types: set[str]
) -> list[dict]:
    if is_sequence_type(field_type):
        return append_conversions_for_template(
            namespace, is_sequence_type(field_type), conversions, processed_types
        )
    elif "/" in field_type:
        if namespace != "":
            current_proto_full_name = (
                namespace
                + "::"
                + field_type.split("/")[0]
                + "__"
                + field_type.split("/")[1]
            )
        else:
            current_proto_full_name = (
                "protolink__"
                + field_type.split("/")[0]
                + "__"
                + field_type.split("/")[1]
                + "::"
                + field_type.split("/")[0]
                + "__"
                + field_type.split("/")[1]
            )

        # Check for duplicates in Protobuf type names
        if current_proto_full_name in processed_types:
            return conversions

        processed_types.add(current_proto_full_name)

        if namespace != "":
            builtin_types = []
            user_types = []
            array_types = []
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
                        processed_types,
                    )
                else:
                    builtin_types.append(field_name_in_child)
            conversions.append(
                {
                    "proto": current_proto_full_name,
                    "ros2": field_type.split("/")[0]
                    + "::msg::"
                    + field_type.split("/")[1],
                    "members": {
                        "builtin_types": builtin_types,
                        "user_types": user_types,
                        "array_types": array_types,
                    },
                }
            )
        else:
            builtin_types = []
            user_types = []
            array_types = []
            fields = get_message_fields(field_type)
            for field_name_in_child, field_type_in_child in fields.items():
                if is_sequence_type(field_type_in_child):
                    array_types.append(field_name_in_child)
                elif "/" in field_type_in_child:
                    user_types.append(field_name_in_child)
                else:
                    builtin_types.append(field_name_in_child)
            conversions.append(
                {
                    "proto": current_proto_full_name,
                    "ros2": field_type.split("/")[0]
                    + "::msg::"
                    + field_type.split("/")[1],
                    "members": {
                        "builtin_types": builtin_types,
                        "user_types": user_types,
                        "array_types": array_types,
                    },
                }
            )
        return conversions
    else:
        return conversions


def to_proto_message_definition(
    field_type: str, field_name: str, message_index: int, is_repeated: bool
) -> str:
    if is_sequence_type(field_type):
        return to_proto_message_definition(
            is_sequence_type(field_type), field_name, message_index, True
        )
    elif "/" in field_type:
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
                field_type_in_child,
                field_name_in_child,
                message_index_in_child,
                is_repeated,
            )

            message_index_in_child = message_index_in_child + 1
        base_proto_string = base_proto_string + "}\n"

        if is_repeated:
            return (
                base_proto_string
                + "\n"
                + "repeated "
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
        proto_string = to_proto_type(field_type)
        assert proto_string is not None, (
            "Can not convert from ros2 message to proto message"
        )
        return proto_string + " " + field_name + " = " + str(message_index) + ";\n"


def get_message_structure(
    msg_type_name: str, output_file: str, header_file: str, source_file: str
) -> None:
    env = Environment(
        loader=FileSystemLoader(searchpath=os.path.dirname(os.path.abspath(__file__)))
    )
    template_header = env.get_template("template_converter.hpp.jinja")
    template_cpp = env.get_template("template_converter.cpp.jinja")

    processed_types = set()
    conversions = append_conversions_for_template(
        "", msg_type_name, [], processed_types
    )

    name = msg_type_name.split("/")[1]
    ros2_message_header = ""
    if name.isupper():
        ros2_message_header = name.lower()
    else:
        for splited in re.split(r"(?=[A-Z])", name):
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
        "conversion_namespace": "protolink__" + msg_type_name.replace("/", "__"),
        "conversion_header": header_file,
    }

    fields = get_message_fields(msg_type_name)

    # print(f"Message: {msg_type_name}")
    # print("Fields:")

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
            processed_types,
        )
        proto_string = proto_string + to_proto_message_definition(
            field_type, field_name, message_index, False
        )
        message_index = message_index + 1
    proto_string = proto_string + "}"

    # Generate proto file
    # print("\nProto file => \n")
    # print(proto_string)
    with open(output_file, mode="w") as f:
        f.write(proto_string)

    # Generate conversion library with Jinja
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
