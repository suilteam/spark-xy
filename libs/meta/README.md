# `suil::meta`
Meta is compiler plugin that can be used to transform that's adds support for
reflection to C++ by reading attributes from types at compile time and generating
metadata for the tagged entities.

## Builtin Attributes
### `meta::serializable`
This attribute is applicable to classes or structs only and is
used to direct the plugin to generate metadata for the tagged type. The attribute
takes an optional `string` parameter which lists the codecs to generate for the
type.
In the example below, the struct `Player` is marked as serializable and supports
`Json` and `Protobuf` codecs.

```c++
[[meta::object("Json|Protobuf")]]
struct Player {
};
```

### `meta::senum`
All enums tagged with the meta enum will have their metadata generated. The
generated metadata simply allows enums to be converted to and from strings.

```c++
// color.hpp
[[meta::senum]]
enum Color {
    RED,
    BLUE
};

// #include <generated/color.hpp>
Color color = Color::RED;
auto name = Enum::toString(color);
std::cout << "Color is: " << name << "\n";
// Runtime
auto color2 = Enum<Color>(name);
assert(color == color2);
```

### `meta::id`
This attribute is applicable to type member variables only. This attribute is
used to assign unique ID's to type members that need to be serializable. The
attribute requires a single integer parameter which must be unique from all the
ID's assigned to other members.

```c++
[[meta::serializable("Wire")]]
struct Player {
    [[meta::id(1)]]
    std::string uid;
    
    [[meta::id(2)]]
    double  power;
};
```