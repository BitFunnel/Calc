#include <iostream>
#include <math.h>
#include <map>
#include <stdexcept>
#include <string>
#include <sstream>


class Calc
{
public:
    Calc(std::string const & src);

    double Evaluate();

    //
    // ParseError records the character position and cause of an error
    // during parsing.
    //
    class ParseError : public std::runtime_error
    {
    public:
        ParseError(char const * message, size_t position);

        friend std::ostream& operator<< (std::ostream &out, const ParseError &e);

    private:
        // Character position where error occurred.
        size_t m_position;
    };

private:
    // Parses an expression of the form
    // EXPRESSION:
    //   SUM
    double ParseExpression();

    // Parses expressions of form
    // SUM:
    //   PRODUCT ('+' PRODUCT)*
    //   PRODUCT ('-' PRODUCT)*
    double ParseSum();

    // Parses expressions of the form
    // PRODUCT:
    //   TERM ('*' TERM)*
    //   TERM ('/' TERM)*
    double ParseProduct();

    // Parses expressions of the form
    // TERM:
    //   (SUM)
    //   CONSTANT
    //   IDENTIFIER
    double ParseTerm();

    // Parses expressions of the form
    // IDENTIFIER:
    //   SYMBOL
    //   SYMBOL(SUM)
    double ParseIdentifier();

    // Parses expressions of the form
    // CONSTANT:
    //
    // DESIGN NOTE: Although the first two DIGIT productions have *, in fact,
    // one of the two must have at least one digit.
    //
    //   [ '+' | '-' ] (DIGIT)* [ '.' DIGIT*] [ ('e' | 'E') [ '+' | '-' ] DIGIT+ ]
    double GetConstant();

    // Parses expressions of the form
    // SYMBOL: ALPHA (ALPHA | DIGIT)*
    std::string GetSymbol();

    // Returns true if current position is the first character of a floating
    // point number.
    bool IsFirstCharOfDouble(char c);

    // Advances the current position past whitespace (space, tab, carriage
    // return, newline).
    void SkipWhite();

    // Attempts to advance past next character.
    // Throws a ParseError if the next character is not equal to
    // the expected character.
    void Consume(char expected);

    // Advances past the next character.
    // Returns the character or '\0' if at the end of the stream.
    char GetChar();

    // Returns the next character without advancing.
    // Returns '\0' if at the end of the stream.
    char PeekChar();

    // Source string to be parsed.
    std::string const m_src;

    // Current position of parser in the m_src.
    size_t m_currentPosition;

    std::map<std::string, double> m_constants;

    typedef double(*Function)(double);
    std::map<std::string, Function> m_functions;
};


Calc::Calc(std::string const & src)
    : m_src(src),
      m_currentPosition(0)
{
    // Initialize symbol table for named constants.
    m_constants["e"] = exp(1);
    m_constants["pi"] = atan(1) * 4;

    // Initialize symbol table for named functions.
    m_functions["cos"] = cos;
    m_functions["sin"] = sin;
    m_functions["sqrt"] = sqrt;
}


double Calc::Evaluate()
{
    return ParseExpression();
}


double Calc::ParseExpression()
{
    double expression = ParseSum();

    SkipWhite();
    if (PeekChar() != '\0')
    {
        throw ParseError("Syntax error.", m_currentPosition);
    }

    return expression;
}


double Calc::ParseSum()
{
    double left = ParseProduct();

    SkipWhite();
    if (PeekChar() == '+')
    {
        GetChar();
        double right = ParseProduct();

        return left + right;
    }
    else if (PeekChar() == '-')
    {
        GetChar();
        double right = ParseProduct();

        return left - right;
    }
    else
    {
        return left;
    }
}


double Calc::ParseProduct()
{
    double left = ParseTerm();

    SkipWhite();
    if (PeekChar() == '*')
    {
        GetChar();
        double right = ParseSum();

        return left * right;
    }
    else if (PeekChar() == '/')
    {
        GetChar();
        double right = ParseSum();

        return left / right;
    }
    else
    {
        return left;
    }
}


double Calc::ParseTerm()
{
    SkipWhite();

    char next = PeekChar();
    if (next == '(')
    {
        GetChar();

        double result = ParseSum();

        SkipWhite();
        Consume(')');

        return result;
    }
    else if (IsFirstCharOfDouble(next))
    {
        return GetConstant();
    }
    else if (isalpha(next))
    {
        return ParseIdentifier();
    }
    else
    {
        throw ParseError("Expected a number, symbol or parenthesized expression.",
                         m_currentPosition);
    }
}


double Calc::ParseIdentifier()
{
    std::string symbol = GetSymbol();

    SkipWhite();
    if (PeekChar() == '(')
    {
        auto it = m_functions.find(symbol);
        if (it != m_functions.end())
        {
            Consume('(');
            double parameter = ParseSum();
            Consume(')');
            return it->second(parameter);
        }
        else
        {
            std::stringstream message;
            message << "Unknown function \"" << symbol << "\".";
            throw ParseError(message.str().c_str(), m_currentPosition);
        }
    }
    else
    {
        auto it = m_constants.find(symbol);
        if (it != m_constants.end())
        {
            return it->second;
        }
        else
        {
            std::stringstream message;
            message << "Unknown symbol \"" << symbol << "\".";
            throw ParseError(message.str().c_str(), m_currentPosition);
        }
    }
}


double Calc::GetConstant()
{
    // s will hold a string of floating point number characters that will
    // eventually be passed to stof().
    std::string s;

    SkipWhite();

    //
    // Gather in s the longest possible sequence of characters that will
    // parse as a floating point number.
    //

    // Optional leading '+' or '-'.
    if (PeekChar() == '+' || PeekChar() == '-')
    {
        s.push_back(GetChar());
    }

    // Optional mantissa left of '.'
    while (isdigit(PeekChar()))
    {
        s.push_back(GetChar());
    }

    // Optional portion of mantissa right of '.'.
    if (PeekChar() == '.')
    {
        s.push_back(GetChar());
        while (isdigit(PeekChar()))
        {
            s.push_back(GetChar());
        }
    }

    // Optional exponent.
    if (PeekChar() == 'e' || PeekChar() == 'E')
    {
        s.push_back(GetChar());

        // Optional '+' or '-' before exponent.
        if (PeekChar() == '+' || PeekChar() == '-')
        {
            s.push_back(GetChar());
        }

        if (!isdigit(PeekChar()))
        {
            throw ParseError("Expected exponent in floating point constant.",
                             m_currentPosition);
        }

        while (isdigit(PeekChar()))
        {
            s.push_back(GetChar());
        }
    }

    // Parse s into a floating point value.
    try
    {
        return stod(s);
    }
    catch (std::invalid_argument)
    {
        throw ParseError("Invalid float.",
                         m_currentPosition);
    }
}


std::string Calc::GetSymbol()
{
    std::string symbol;

    SkipWhite();
    if (!isalpha(PeekChar()))
    {
        throw ParseError("Expected alpha character at beginning of symbol.",
                         m_currentPosition);
    }
    while (isalnum(PeekChar()))
    {
        symbol.push_back(GetChar());
    }
    return symbol;
}


bool Calc::IsFirstCharOfDouble(char c)
{
    return isdigit(c) || (c == '-') || (c == '+') || (c == '.');
}


void Calc::SkipWhite()
{
    while (isspace(PeekChar()))
    {
        GetChar();
    }
}


void Calc::Consume(char c)
{
    if (PeekChar() != c)
    {
        // TODO: REVIEW: Check lifetime of c_str() passed to exception constructor.
        std::stringstream message;
        message << "Expected '" << c << "'.";
        throw ParseError(message.str().c_str(), m_currentPosition);
    }
    else
    {
        GetChar();
    }
}


char Calc::GetChar()
{
    char result = PeekChar();
    if (result != '\0')
    {
        ++m_currentPosition;
    }
    return result;
}


char Calc::PeekChar()
{
    if (m_currentPosition >= m_src.length())
    {
        return '\0';
    }
    else
    {
        return m_src[m_currentPosition];
    }
}


//*****************************************************************************
//
// Calc::ParseError
//
//*****************************************************************************
Calc::ParseError::ParseError(char const * message, size_t position)
  : std::runtime_error(message),
    m_position(position)
{
}


std::ostream& operator<< (std::ostream &out, const Calc::ParseError &e)
{
    out << std::string(e.m_position, ' ') << '^' << std::endl;
    out << "Calc error (position = " << e.m_position << "): ";
    out << e.what();
    out << std::endl;
    return out;
}



//*****************************************************************************
//
// Test() runs a number of test cases for the parser.
// It prints a summary of each case's input and output and either "OK"
// or "FAILED" depending on whether the test succeeded or failed.
//
// Returns true if all tests pass. Otherwise returns false.
//
//*****************************************************************************
bool Test()
{
    class TestCase
    {
    public:
        TestCase(char const * input, double output)
            : m_input(input),
              m_output(static_cast<double>(output))
        {
        }


        bool Run(std::ostream& output)
        {
            bool succeeded = false;

            output << "\"" << m_input << "\" ==> ";

            try {
                Calc parser(m_input);
                double result = parser.Evaluate();

                output << result;

                if (result == m_output)
                {
                    succeeded = true;
                    output << " OK";
                }
                else
                {
                    output << " FAILED: expected " << m_output;
                }
            }
            catch (...) {
                output << "FAILED: exception.";
            }

            output << std::endl;

            return succeeded;
        }

    private:
        char const * m_input;
        double m_output;
    };


    TestCase cases[] =
    {
        // Constants
        TestCase("1", 1.0),
        TestCase("1.234", 1.234),
        TestCase(".1", 0.1),
        TestCase("-2", -2.0),
        TestCase("-.1", -0.1),
        TestCase("1e9", 1e9),
        TestCase("2e-8", 2e-8),
        TestCase("3e+7", 3e+7),
        TestCase("456.789e+5", 456.789e+5),

        // Symbols
        TestCase("e", static_cast<double>(exp(1))),
        TestCase("pi", static_cast<double>(atan(1) * 4)),

        // Addition
        TestCase("1+2", 3.0),
        TestCase("3+e", 3.0 + static_cast<double>(exp(1))),

        // Subtraction
        TestCase("4-5", -1.0),

        // Multiplication
        TestCase("2*3", 6.0),

        // Parenthesized expressions
        TestCase("(3+4)", 7.0),
        TestCase("(3+4)*(2+3)", 35.0),

        // Combinations
        TestCase("1+-2", -1.0),     // Addition combined with unary negation.

        // White space
        TestCase("\t 1  + ( 2 * 10 )    ", 21.0),

        // sqrt
        TestCase("sqrt(4)", 2.0),
        TestCase("sqrt((3+4)*(2+3))", sqrt(35)),
        TestCase("sqrt(1 + 2 )", sqrt(3)),

        // trig
        TestCase("cos(pi)", -1),
        TestCase("sin(0)", 0),
    };


    bool success = true;
    for (size_t i = 0; i < sizeof(cases) / sizeof(TestCase); ++i)
    {
        success &= cases[i].Run(std::cout);
    }

    return success;
}


int main()
{
    std::cout << "Running test cases ..." << std::endl;
    bool success = Test();
    if (success)
    {
        std::cout << "All tests succeeded." << std::endl;
    }
    else
    {
        std::cout << "One or more tests failed." << std::endl;
    }

    std::cout << std::endl;
    std::cout << "Type an expression and press return to evaluate." << std::endl;
    std::cout << "Enter an empty line to exit." << std::endl;

    std::string prompt(">> ");

    for (;;)
    {
        std::string line;
        std::cout << prompt << std::flush;
        std::getline(std::cin, line);

        // TODO: Should really see if line is completely blank.
        // Blank lines cause the parser to crash.
        if (line.length() == 0)
        {
            break;
        }

        try
        {
            Calc parser(line);
            double result = parser.Evaluate();
            std::cout << result << std::endl;
        }
        catch (Calc::ParseError& e)
        {
            std::cout << std::string(prompt.length(), ' ');
            std::cout << e;
        }
    }

    return 0;
}
