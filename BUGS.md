## This is a current list of Bugs that need to be fixed.
- In global variables, the initializer part (if it's a variable reference) causes an internal code generation error, that is because C doesn't allow non-constant initializers in global variables, and we use C to generate output code. This should be fixed soon.
