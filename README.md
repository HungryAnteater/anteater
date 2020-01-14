# anteater

Features
---------------------------------------------------
- Assignment
- Array assignment
- All basic operators
- If/then
- While loops
- Functions + return values
- Locals
- Ints, Floats, Strings, and Arrays
- Nested comments
- String contatenation between strings, ints, and floats
- Array construction, access, and assignment
- Nested functions / local functions

Quirks and Limitations
---------------------------------------------------
- whiles, if/thens, and functions must be followed by
  semicolon
- all functions must end with "return;" or
  "return <some exp>;"

Syntax
---------------------------------------------------
Very similar to C, Lua, Squirrel, etc.

How to Run
---------------------------------------------------
To run examples, the arguments to the main function
need to be changed (I do it in the debug tab for
for project.)  The first argument is the code file,
the second is the output file.

BNF for the AntEater Scripting Language
---------------------------------------------------
program     ::= { statement ";" | function }

statement   ::= declaration | exp | ifthen | while | dowhile | "break" | "return" [ exp ]
                block | assignment

exp         ::= exp2 [ ("and" | "&&" | "or" | "||") exp ]

exp2        ::= exp3 [ ("==" | "!=" | "<" | "<=" | ">" | ">=") exp2 ]

exp3        ::= exp4 [ ("+" | "-") exp3 ]

exp4        ::= factor [ ("*" | "/" | "%") exp4 ]

call        ::= factor "(" [ exp { "," exp } ] ")"

factor      ::= "(" exp ")" | NUMBER | STRING | IDENTIFIER |
                "true" | "false" | "null" | "-" factor | ("!" | "not") exp |
                call | array | function

block       ::= "{" { statement ";" } "}"

assignment  ::= IDENTIFIER "=" exp

declaration ::= "local" idlist [ "=" explist ]

preop       ::= "+" | "-" | ["not" | "!"]

function    ::= "function" [ IDENTIFIER ] "(" [ IDENTIFIER { "," IDENTIFIER } ] ")" block

array       ::= "[" explist "]"

ifthen      ::= "if" "(" exp ")" statement [ "else" statement ]

while       ::= "while" "(" exp ")" statement

dowhile     ::= "do" statement "while" exp

foreach     ::= "foreach" "(" IDENTIFIER "in" exp ")" statement
