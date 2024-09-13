# Coding Standards and Best Practices to Follow

## Table of contents
1. [Coding Standards](#Coding_Standards)
2. [Coding Conventions](#Coding_Conventions)
3. [Common coding conventions](#Common_coding_conventions)
4. [Formatting](#Formatting)
     1. [Base Style](#Base_Style)
     2. [Style Customization](#Style_Customization)
     3. [Excluding code blocks from formatting](#Excluding_code_blocks_from_formatting)
     4. [Code formatting tools](#Code_formatting_tools)
5. [Naming Conventions](#Naming_Conventions)
6. [Braces and Parentheses](#Braces_and_Parentheses)
7. [Comments](#Comments)
8. [Whitespace](#Whitespace)
9. [Code Line Length](#Code_Line_Length)
10. [Function/Method Length](#Function_Method_Length)
11. [Error Handling](#Error_Handling)    
12. [Links](#Links)


<a id="Coding_Standards"></a>
## Coding Standards 
Coding standards (coding guidelines or programming style guides) are a set of rules and conventions that developers follow while writing code.

<a id="Coding_Conventions"></a>
## Coding Conventions 
Coding conventions are programming language-specific guidelines that offer recommendations for keeping your code clean.  
These recommendations focus mainly on programming style and organization best practices.  
Different programming languages and communities may have their specific conventions.  

<a id="Common_coding_conventions"></a>
## Common coding conventions followed across various languages
- **Formatting**:  
    Be consistent in formatting throughout the codebase.

- **Indentation**:  
    Use consistent indentation

- **Naming Conventions**:  
    - Use descriptive names for variables, functions, etc.  
    - Follow a consistent naming convention (e.g., CamelCase, snake_case)

- **Braces and Parentheses**:  
    Use consistent placement of braces and parentheses

- **Comments**:  
    - Write clear and concise comments for complex code sections  
    - Avoid unnecessary comments that state the obvious  
    - Be specific and concise

- **Whitespace**:  
    - Use whitespace effectively to enhance code readability.
    - Avoid excessive or unnecessary whitespace.

- **Code Line Length**:  
    - Limit the length of lines to improve readability.
    - Keep lines of code within a reasonable length (commonly 80-120 characters) to avoid horizontal scrolling. 

- **Function/Method Length**:  
    - Keep functions/methods short and focused on a specific task.
    - Long functions should be broken down into smaller, more manageable ones.

- **Error Handling**: 
    - Implement proper error handling and communicate errors clearly. 
    - The goal is to identify and properly respond to errors and exceptions.
    - When you have clear guidelines for reporting, logging, and handling errors, your code becomes more reliable and, over time, more error-proof.

- **File Organization**:  
    Organize code into logical sections or modules. Follow a consistent file and directory structure.

<a id="Formatting"></a>
## Formatting

<a id="Base_Style"></a>
### Base Style
The formatting style is based on Google's style ( A style complying with [Google’s C++ style guide](https://google.github.io/styleguide/cppguide.html))

<a id="Style_Customization"></a>
### Style Customization

#### Indentation
- Use only spaces, and indent **4 spaces** at a time
- We use spaces for indentation!
- Do not use tabs in your code!

#### Conditional Statements
- the opening curly brace `{` is placed at the end of the line after the control statement (K&R style)
- there is no space between the keyword if and the opening parenthesis `(`
- there should be no space between the opening parenthesis `(` and the variable or expression that follows it

##### if statement
```c
if(a < 0){
    do_something();
}
```
##### else statement
```c
if(condition1){
    // code if condition1 is true
}
else if(condition2){
    // code if condition2 is true
}
else{
    // code if both conditions are false
} 
```
##### switch statement
```c
switch(variable){
    case value1:
        // code for value1
        break;
    case value2:
        // code for value2
        break;
    default:
        // code if no cases match
}
```

#### Looping Statements
```c
for(initialization; condition; increment){
    // code to execute
}
```

```c
while(condition){
    // code to execute
}
```

```c
do{
    // code to execute
} while(condition)
```

#### Function definitions
```c
int do_something(int a, int b)
{
    int x = a + b;
    if(x > 0){
        printf("positive\n");
    }
    else{
        printf("negative or zero\n");      
    }
    return x;
}
```
The opening curly brace `{` is placed on a new line after the function declaration, rather than at the end of the line (Allman style).  
Return type is placed on the same line as function name.  
Parameters are placed on the same line if they fit.  

<a id="Excluding_code_blocks_from_formatting"></a>
### Excluding code blocks from formatting
To preserve specific formatting for certain lines or blocks of code, is using **clang-format's** `// clang-format off` and `// clang-format on` comments to disable formatting for those sections.

```c
// clang-format off
int array[] = {1, 2, 3, 4, 5, 6, 7, 8};
// clang-format on
```

<a id="Code_formatting_tools"></a>
### Code formatting tools
#### Clang-format
- It is recommended to use the clang-format tool for formatting the project's code.  
- Use clang-format to ensure consistent formatting throughout the codebase.  
- The **.clang-format** configuration file for the clang-format tool, containing a description of the code formatting style, is located in the main directory of the project.

## Naming Conventions <a id="Naming_Conventions"></a>
- The **snake_case** convention for function names.
- All letters are lowercase, and words are separated by underscores.
- Choose names that are descriptive and meaningful, making it easier for others (and yourself) to understand the code.
- Use meaningful names for variables and functions to make the code self-documenting.
- It is better to avoid the use of digits in variable names.

### Local Variables
- Convention: `snake_case`
- Example:

```c
void calculate_area(float radius) {
    float area = 3.14 * radius * radius;
    // Use area for further calculations
}
```

### Global Variables
- Convention: **`snake_case`** (often prefixed with g_ to indicate global scope)
- Example:

```c
  int g_max_users = 100;
```

### Constants
- Convention: **`UPPER_SNAKE_CASE`**
- Example:

```c
#define MAX_CONNECTIONS 10
#define PI 3.14159
```

### Functions
- Convention: **`snake_case`**
- Example:

```c
void fetch_data_from_api(const char* api_url) {
    // Function implementation
}
```

### Summary of Conventions
- Local Variables: **`snake_case`**
- Global Variables: **`snake_case`** (or `g_snake_case`)
- Constants: **`UPPER_SNAKE_CASE`**
- Functions: **`snake_case`**

## Braces and Parentheses <a id="Braces_and_Parentheses"></a>
### Functions
- The open parenthesis is always on the same line as the function name.
- There is never a space between the function name and the open parenthesis.
- There is never a space between the parentheses and the parameters.
- The open curly brace is never on the end of the last line of the function declaration, it is always on the start of the next line.
- The close curly brace is either on the last line by itself or on the same line as the open curly brace.
- There should be a space between the close parenthesis and the open curly brace.
- Default indentation is 4 spaces.
- Wrapped parameters have a 4 space indent.

```c
int main(int argc, char *argv[])
{
    while(x == y){
        do_something();
        do_something_else();

        if(some_error){
            fix_issue(); // single-statement block without braces
        }
        else{
            continue_as_usual();
        }
    }
    final_thing();
}
```

## Comments  <a id="Comments"></a>
Use comments to explain complex logic, especially within blocks of code.

### Single-line Comments 
Use // to comment out a single line.

```c
// This is a single-line comment
int x = 5; // Initialize x to 5
```
### Multi-line Comments 
Use /* ... */ to comment out multiple lines

```c
/*
 * This is a multi-line comment.
 * It can span multiple lines.
 */
int y = 10;
```

### Best Practices for Commenting

- **Be Clear and Concise**: Comments should be easy to understand and to the point. Avoid unnecessary jargon.

- **Explain Why, Not What**: Focus on explaining the reasoning behind complex logic rather than stating what the code does. The code itself should be self-explanatory when possible.
    ```c
    // Calculate the area of a circle using the formula A = πr²
    double area = M_PI * radius * radius;
    ```

- **Use Comments to Describe Functionality**: At the beginning of functions, provide a brief description of what the function does, its parameters, and its return value. 
    ```c
    /**
     * Calculates the factorial of a number.
     * @param n The number to calculate the factorial for.
     * @return The factorial of the number.
     */
    int factorial(int n) {
        // Implementation here
    }
    ```

- **Avoid Redundant Comments**: Do not comment on obvious code. For example, commenting int x = 5; // Set x to 5; is unnecessary.

- **Update Comments:** Ensure comments are updated when the code changes. Outdated comments can be misleading.

- **Use TODO Comments:** If there are areas in the code that need improvement or further work, use TODO comments to highlight them.
    ```c
    // TODO: Optimize this function for better performance
    double area = M_PI * radius * radius;
    ```

- **Document Assumptions and Limitations:** If your code relies on certain assumptions or has limitations, document them clearly.
    ```c
    // Assumes input is always a positive integer
    ```

- **Consistent Style**: Maintain a consistent commenting style throughout your codebase. This includes formatting, capitalization, and punctuation.


### Function declarations
Almost every function declaration should have comments immediately preceding it that describe what the function does and how to use it. 
```c
/**
 * Calculates the factorial of a number.
 * @param n The number to calculate the factorial for.
 * @return The factorial of the number.
 */
int factorial(int n) {
    // Implementation here
}
```

### Variable Comments
The actual name of the variable should be descriptive enough to give an idea of what the variable is used for.  
In certain cases, more comments are required.

### Global Variables
All global variables should have a comment describing what they are, what they are used for. 

## Whitespace <a id="Whitespace"></a>
- Spacing Around Operators
    Use spaces around operators (e.g., `+, -, =, ==`) to improve readability.
- Spacing in Function Calls
    - When calling functions, avoid spaces between the function name and the opening parenthesis.
    - Use spaces after commas in function arguments for clarity.
- Avoid Trailing Whitespace

## Code Line Length <a id="Code_Line_Length"></a>
- A common limit is between 80 to 120 characters per line. 
- Shorter lines are generally easier to read. If a line of code is too long, it can be difficult to understand at a glance. Breaking long lines into shorter ones can enhance clarity.

## Function/Method Length <a id="Function_Method_Length"></a>
- Aim to keep functions short and focused on a single task or responsibility.
- A common guideline is to limit functions to around 20-30 lines of code, although this can vary based on the complexity of the task and the programming language.
- Each function should have a single responsibility or purpose. 
- Shorter functions are generally easier to read and understand.
- If you find that a function is becoming too long, consider refactoring it.

## Error Handling <a id="Error_Handling"></a>
- Return Error Codes
    - Use return values to indicate success or failure. Define clear error codes and document them.
- Log Errors
    - When handling errors, provide clear and meaningful error messages that help users understand what went wrong and how to fix it.

## Links <a id="Links"></a>
- [https://clang.llvm.org/docs/ClangFormat.html](https://clang.llvm.org/docs/ClangFormat.html)
- [https://clang.llvm.org/docs/ClangFormatStyleOptions.html](https://clang.llvm.org/docs/ClangFormatStyleOptions.html)
- [https://google.github.io/styleguide/cppguide.html](https://google.github.io/styleguide/cppguide.html)
- [https://blog.codacy.com/coding-standards](https://blog.codacy.com/coding-standards)