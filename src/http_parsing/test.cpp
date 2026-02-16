#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cctype>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>

enum TokenType
{
    Number,
    Identifier,
    Equals,
    OpenParen,
    CloseParen,
    BinaryOperator,
    Let,
};

struct Token
{
    std::string value;
    TokenType type;
};

// Find reserved identifier
typedef std::map<std::string, TokenType> ReservedIdentMap;
ReservedIdentMap reservedIdent;

void INIT_RESERVED_IDENTIFIER()
{
    reservedIdent["let"] = TokenType::Let;
}

// function to spilt string with space " "
std::vector<std::string> splitString(const std::string &sourceCode)
{
    std::vector<std::string> words;
    std::string word;
    for (char ch : sourceCode)
    {
        if (ch != ' ')
        {
            word += ch;
        }
        else if (!word.empty())
        {
            words.push_back(word);
            word.clear();
        }
    }
    if (!word.empty())
    {
        words.push_back(word);
    }
    return words;
}

long long SHIFT_CURR = 0;
std::string shift(std::vector<std::string> &src)
{
    std::string current = src.front();
    src.erase(src.begin());
    return current;
}

bool isNumber(const std::string &str)
{
    for (char ch : str)
    {
        if (!isdigit(ch))
            return false;
    }
    return true;
}

bool isAlpha(const std::string &str)
{
    for (char ch : str)
    {
        if (!isalpha(ch))
            return false;
    }
    return true;
}

bool isSkippable(char ch)
{
    return ch == ' ' || ch == '\t' || ch == '\n';
}

void printRandom()
{
    std::cout << SHIFT_CURR << std::endl;
}

Token token(std::string value, TokenType tokentype)
{
    return {value, tokentype};
}

std::vector<Token> tokenize(std::string &sourceCode)
{
    std::vector<Token> tokens;
    std::vector<std::string> src = splitString(sourceCode);

    // build each token untill the end of the file
    while (!src.empty())
    {
        if (src.front() == "(")
        {
            tokens.push_back(token(shift(src), TokenType::OpenParen));
        }
        else if (src.front() == ")")
        {
            tokens.push_back(token(shift(src), TokenType::CloseParen));
        }
        else if (src.front() == "+" || src.front() == "-" || src.front() == "*" || src.front() == "/")
        {
            tokens.push_back(token(shift(src), TokenType::BinaryOperator));
        }
        else if (src.front() == "=")
        {
            tokens.push_back(token(shift(src), TokenType::Equals));
        }
        else
        { // Handle multicharacter token
            // Handling number tokens
            if (isNumber(src.front()))
            {
                std::string number;
                while (!src.empty() && isNumber(src.front()))
                {
                    number += shift(src);
                }

                tokens.push_back(token(number, TokenType::Number));
            } // Handling Identifier tokens
            else if (isAlpha(src.front()))
            {
                std::string ident = shift(src);
                // check for reserved tokens like let etc
                ReservedIdentMap::iterator it = reservedIdent.find(ident);
                if (it != reservedIdent.end())
                {
                    tokens.push_back(token(ident, it->second));
                }
                else
                {
                    tokens.push_back(token(ident, TokenType::Identifier));
                }
            } // Handling skippable tokens like ' ' || \n || \t
            else if (isSkippable(src.front()[0]))
            {
                shift(src);
            }
            else
            {
                std::cout << "Unrecognized character found! " << std::endl;
                exit(1);
            }
        }
    }

    return tokens;
}

int main(int argc, char *argv[])
{
    // Check if the arguments are correct
    if (argc != 2)
    {
        std::cerr << "Incorrect arguments" << std::endl;
        std::cerr << "Correct usage: ./dejavu <input file path --> input.vu>" << std::endl;
        return EXIT_FAILURE;
    }

    // checking if the input file is valid -> .vu ?
    {
        const char *ext = ".vu";
        size_t xlen = strlen(ext);
        size_t slen = strlen(argv[1]);
        int found = strcmp(argv[1] + slen - xlen, ext) == 0;
        if (found == 0)
        {
            std::cerr << "Invalid code file" << std::endl;
            return EXIT_FAILURE;
        }
    }

    // initializing registered keywords
    INIT_RESERVED_IDENTIFIER();

    //reading the input source code file and converting it into a string stream
    std::string sourceCode;
    {
        std::stringstream contents_stream;
        std::fstream input(argv[1], std::ios::in);
        contents_stream << input.rdbuf();
        sourceCode = contents_stream.str();
    }

    std::vector<Token> tokens = tokenize(sourceCode);
    for (int i = 0; i < tokens.size(); ++i)
    {
        std::cout << "Value: " << tokens[i].value << "   Type: " << tokens[i].type << std::endl;
    }
}