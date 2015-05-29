# protogen

In short, protogen is an utility that takes messages and protocols definitions
and generates source code by populating text templates with data from these definitions.


protogen itself is completely language neutral, but since '$' sign is used a lot
in templates, I guess it would be somewhat cumbersome to use it for perl and php.


You can generate source code for several languages at once (from the same .def file).
We used it quite a lot to make protocol between java and C++ applications.


protogen uses following abstractions:

 * protocol is a collection of messages, where each message can have additional 'protocol level' properties (like in, out, inout).
 * message is a collection of fields, where each property have tag, type, name and can have additinal properties (like optional, or array).
 * type of a field is one of defined types, another message or enum.
 * type definition looks more like declaration. You just specify that there is a type with given name.
 * enum have an underlying type and collection of items, where each item have type and value.
 * fieldset is a collection of fields with predefined tag and properties.

By default protogen generates source code for each defined protocol, all messages in all protocols, all used enums and used fieldsets.
But it is possible to generate only selected few messages or only specified fieldset.
