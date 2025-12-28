```<Program> ::= { <TopLevelDecl> }

<TopLevelDecl> ::= <ImportDecl> | <TypeDecl> | <InstanceDecl>

<ImportDecl> ::= "import" <DottedIdentifier>

<DottedIdentifier> ::= <Identifier> { "." <Identifier> }

<TypeDecl> ::= "class" <Identifier> "{" { <MemberDecl> } "}"

<MemberDecl> ::= <PropertyDecl>
               | <EventDecl>
               | <MethodDecl>

<PropertyDecl> ::= "property" <Type> <Identifier> ":" <Expr> ";"

<EventDecl> ::= "event" <Identifier> "{" <EnumVariantList> "}" 

<MethodDecl> ::= "method" <Type>? <Identifier> "(" [ <ParamList> ] ")" "{" { <Stmt> } "}"

<ParamList> ::= <Param> { "," <Param> }

<Param> ::= <Type> <Identifier>

<InstanceDecl> ::= <Identifier> ":" <Type> "{" { <PropertyInit> | <InstanceDecl> | <WhenStmt> | <AnimateStmt> } "}"

<PropertyInit> ::= <Identifier> ":" <Expr> ";"

<WhenStmt> ::= "when" <Expr> "{" { <WhenArm> } "}"

<AnimateStmt> ::= "animate" "{" { <AnimateProperty> } "}"

<AnimateProperty> ::= <Identifier> ":" <Expr> ";"

// Animation properties:
//   property: <string>    - Property to animate (e.g., "x", "y", "opacity")
//   from: <number>        - Starting value
//   to: <number>          - Ending value
//   duration: <number>    - Duration in milliseconds
//   easing: <string>      - Easing function (e.g., "linear", "ease_in", "bounce")
//   delay: <number>       - Optional delay before starting (ms)
//   repeat: <bool>        - Whether to repeat infinitely

<WhenArm> ::= <Pattern> "=>" <StmtBlock>
           | "_" "=>" <StmtBlock>  // default, allowed only for non-enum types

<Pattern> ::= <Literal> | <Identifier>  // currently single value; can be extended for structured bindings

<StmtBlock> ::= <Stmt> | "{" { <Stmt> } "}"

<Stmt> ::= <PropertyAssignment> 
         | <MethodCall>
         | <WhenStmt>
         | <ExprStmt>

<PreviewExpr>
    ::= "preview" "{" PreviewItem* "}"

<PreviewItem>
    ::= InstanceExpr

<ValueExpr>
    ::= Literal
     |  ReferenceExpr
     |  CallExpr
     |  WhenExpr
     |  PreviewExpr

<PropertyAssignment> ::= <Identifier> ":" <Expr> [ "or" PreviewExpr ]

<MethodCall> ::= <Identifier> "(" [ <ArgList> ] ")" ";"

<ArgList> ::= <Expr> { "," <Expr> }

<ExprStmt> ::= <Expr> ";"

<Expr> ::= <Literal>
         | <Identifier>
         | <PropertyAccess>
         | <MethodCallExpr>
         | <WhenExpr>

<WhenExpr> ::= "when" <Expr> "{" { <WhenArm> } "}"  // value-producing when

<PropertyAccess> ::= <Identifier> "." <Identifier>

<MethodCallExpr> ::= <Identifier> "(" [ <ArgList> ] ")"

<Type> ::= "Int" | "Bool" | "Float" | "Str" | <Identifier> | "Enum" "<" <Identifier> ">"

<Literal> ::= <IntegerLiteral> | <FloatLiteral> | <BoolLiteral> | <StringLiteral> | <EnumLiteral>

<EnumLiteral> ::= <Identifier>  // a variant of an enum type

<EnumVariantList> ::= <Identifier> { "," <Identifier> }

<Identifier> ::= letter { letter | digit | "_" }

<IntegerLiteral> ::= digit { digit }

<FloatLiteral> ::= digit { digit } "." digit { digit }

<BoolLiteral> ::= "true" | "false"

<StringLiteral> ::= '"' { character } '"'
```