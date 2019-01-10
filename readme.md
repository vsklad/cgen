# CGen
CGen is a tool for encoding [SHA-1](https://en.wikipedia.org/wiki/SHA-1) and [SHA-256](https://en.wikipedia.org/wiki/SHA-2) hash functions into [CNF](https://en.wikipedia.org/wiki/Conjunctive_normal_form) in [DIMACS](http://www.satcompetition.org/2009/format-benchmarks2009.html) format, also into [ANF](https://en.wikipedia.org/wiki/Algebraic_normal_form) polynominal system in [PolyBoRi](http://polybori.sourceforge.net) output format. The tool produces compact optimized encodings. The tool allows manipulating CNF encodings further via assignment of variable values and subsequent optimization.

- Project page: <https://cgen.sophisticatedways.net>.
- Source code is published under [MIT license](https://github.com/vsklad/cgen/blob/master/LICENSE).
- Source code is available on GitHub: <https://github.com/vsklad/cgen>.

[![Build Status](https://travis-ci.org/vsklad/cgen.svg?branch=master)](https://travis-ci.org/vsklad/cgen)

## Description
CGen is built to make it easier to analyse [SAT problems](https://en.wikipedia.org/wiki/Boolean_satisfiability_problem) through manipulation of their CNF (DIMACS) and ANF (PolyBoRi) representations. SHA-1/SHA-2 (SHA) encodings are chosen as a reference/example implementation. The tool produces SHA algorithm encodings with different assignments of both message and hash values. Design of the tool is not limited to these algorithms.

Beyond encoding techniques, for variable assignments CGen implements unit propagation, equivalences reasoning and limited binary clauses resolution. This way, it is a simple SAT preprocessor similar to [SatELite](http://minisat.se/SatELite.html) and [Coprocessor](http://tools.computational-logic.org/content/riss.php). CGen does not implement any variant of [DPLL algorithm](https://en.wikipedia.org/wiki/DPLL_algorithm) and does not make any decisions with respect to variable values.

CGen implements three notable features.

### Compact encoding
The encoding is optimized to minimize both number of variables and number of clauses/equations. The assumption is that without redundancies, it may be easier to analyse the problem's complexity and structure. Application of the below techniques results in substantially more compact encodings than those published to date and known to the author. As an example, full SHA-1 is encoded into CNF with 26,156 variables and 127,200 clauses.

1. New variables are introduced only when necessary. For example, rotation, negation, operations where all arguments are constants, require no additional variables.
2. All boolean algebra operations/primitive functions are encoded in the most efficient possible way.
3. For CNF, every combination of bitwise n-nary addition is statically optimized using [Espresso Logic Minimizer](https://en.wikipedia.org/wiki/Espresso_heuristic_logic_minimizer), as a set of pre-defined clauses. This includes simlifications when one or more operands are constants.
4. For boolean algebra operations, any resultant constant values are optimized away during encoding. For example, (x ^ ~x ^ y) is  encoded as (y) without any new clauses or variables generated.
5. Unit propagation, equivalences reasoning and limited binary clauses resolution executed while assigning variables in an existing encoding.

### Named variables
A set of literals and constants can be grouped and given a name.  For example, hash value is a set of 160 literals and constants, depending on the encoding parameters.
Such "named variable" may change throughout subsequent manipulations. CGen tracks these changes. Further, CGen looks for internal structure when a large number of binary variables is grouped together, to produce the most compact representation. For example, {1/32/1}/16/32 describes a set of 16 32-bit words with 512 variables corresponding sequentially to each bit. These descriptions are stored within the DIMACS file as specially formatted comments.
For a SHA encoding, at least two variables are defined, M for message and H for the hash value.

In particular, named variables are specified as a combination of:

1. A 1- or 2-dimentional array of CNF literals and constants. Second dimention allows for mapping arrays of words for SHA algorithms.
2. Sequences of literals and constants where diference (step) between elements is the same.
3. Sequences of binary constants which are grouped into binary and hexadecimal numbers.

### Assignment of parameters
CGen allows defining and assigning named variables via command line. For a new encoding, SHA message if specified, is embedded into it. In all other instances
CGen performs basic validation of the input, then applies the implemented pre-processing techniques to optimise the encoding. Also, it is possible to define named variables by editing the DIMACS file manually.

In particular, CGen supports:

1. Use of binary, hexadecimal and character-sequence constants (with bits mapped to binary variables).
2. Setting of specific bits/binary variables of the named variable.
3. Assigning random values to a subset of randomly chosen binary variables.
4. Padding of 1-block SHA messages.
5. Computing hash value given a constant message, then assigning it along with some (or zero) bits of the message fixed.

Furthermore, it is possible to combine or sequence multiple assignments of the same named variable within the same encoding, running the tool several times with different parameters and analysing the results.

## Usage

### Build
Source code is hosted at GitHub. To obtain and build:

    git clone https://github.com/vsklad/cgen
    make
    
CGen has no external dependencies other than [C++ STL](https://en.wikipedia.org/wiki/Standard_Template_Library). The makefile relies on [GNU g++](https://gcc.gnu.org/) compiler.

### Run
To run the tool please launch "./cgen" for OSX/Linux and "cgen.exe" for Windows.

### Command Line
The tool supports the following parameters:

    cgen encode (SHA1|SHA256) [-f (CNF|ANF)] [-r <rounds>] [-v <name> <value>]... [<encoder options>] <output file name>
    cgen (assign | define) [-v <name> <value>]... <input file name> <output file name>
    cgen --help
    cgen --version

#### Commands

        encode (SHA1|SHA256) - generate the encoding and 
            save it in the chosen format (-f option) to the specified <output file name>
            can generate the encoding for specified number of rounds (-r option),
            also partialy or fully assign the message and hash values (-v option)

        assign - read <input file name>, assign variables as specified and save it to <output file name>
            requires input file in CNF/DIMACS format produced by the same tool
            this is due to variable specifications encoded as DIMACS comments
            these variable specificatiosn may be added to an externally produced file
            using the <define> command

        define - define one or more named variables
            accepts any CNF/DIMACS files, including those produced by other means
            records variable definitions in the output file as DIMACS comments
            "random", "compute" and "except" options are not supported within variable definitions
            
#### Options

        -v <name> <value>
            specification of the named variable, 
            its mapping to binary variables and/or its constant values
            named variable is a sequence of CNF literals and constants represented with bits
            named variable definitions are recorded in the "var" statements 
            in comments within the DIMACS file
            
            <name> is the name of the variable, case sensitive
                may contain characters '0'..'1', 'a'..'z', 'A'..'Z', '_'
                first symbol may not be a digit
                an option with the particular variable may only appear once
            <value> is the variable value or specification
                constant binary variables may be grouped into bytes
                and further represented as a string, a hexadecinal or binary number
                sequential representation can be further optimized as below
                if a value includes spaces, it must surrounded with double-quotes
                to allow the shell process it as a single argument
            supported specifications:
                <value> = ((<hex_value> | <bin_value> | <str_value> | <var_value>) [<pad>][<except_range>]) | 
                     <rnd_value> | compute
                <hex_value> = 0x['0'..'9', 'a'..'f', 'A'..'F']...
                    a big endian hexadecimal constant
                <bin_value> = 0b['0'..'1']...
                    a big endina binary constant
                <str_value> = ([\0x21..\0x7F]... | "[\0x20..\0x7F]...") [<pad>][<except_range>]
                    an ASCII string, with quotes if includes space
                    big endian format is assumed if mapped to word(s)
                <rnd_value> = random:<number>
                    assigns <number> of binary variables (bits) to random values
                    binary variables (bits) are chosen randomly as well
                    <number> must be between 1 and the total number of binary variables
                compute
                    the value is computed and the formula is produced as follows:
                    1. assign other variables to the specified constant values without <exclusion range>
                    2. determe the computed value by encoding/evaluating the formula with those assignments
                    3. encode the formula with all variables and <except_range>
                <pad> = pad:(SHA1 | SHA256)
                    the variable treated as a SHA-1/SHA-256 message and is padded accordingly
                <except_range> = except:<first>..<last>
                    range of binary positions/variables to keep variable, 
                    i.e. ignore the constant assignment of
                    one-based index, i.e. <first> and <last> must be between 1 and the size of the variable
                
                <var_value> = (<literal> | <hex_value> | <bin_value>)[<sequence_clause>]
                <var_value> = "{" <var_value> ["," <var_value>]..."}"[<sequence_clause>] 
                    defines a named variable by describing a set of literals and constants
                <sequence_clause> = "/"<count>["/"<step>]
                    defines a sequence where the first item is associated <var_value>
                    there are <count> elements in the sequence
                    each subsequent item is produced by adding <step> to the previous one
                    for compound items, the same <step> applies to every literal within the item
                    
        -f (CNF|ANF)
            encoding type/form/format to produce, CNF by default (if missing)
            CNF refers to CNF saved in DIMACS format
            ANF refers to ANF saved in PolyBoRi output format
        
        -r <value>
            number of SHA1 rounds to encode
            the value mut be between 1 and 80, defaults to 80 if not specified
            
        -h | --help
            output parameters/usage specification
            
        --version
            output version number of the tool
            
        --normalize_variables | --nv
            reindex variables such that no negations are present 
            within named variable definitions within the output file;
            may add aditional clauses/equations as necessary
        
        <encoder options> [--add_max_args=<value>] [--xor_max_args=<value>]
            these options define how the encoder processes addition and xor's for multiple operands
            these options are allowed for CNF only (not allowed for ANF)
            specify maximal number of binary variables to be encoded together, plus the result
            any constants are optimized out when encoding and are outside this number 
            <add_max_args> is a number between 2 and 6, 3 if not specified
                defines maximal number of binary variables to be added together
            <xor_max_args> is a number between 2 and 10, 3 if not specified
                defines maximal number of binary variables to be xor'ed together
        
### Pre-defined Variables
Two variables are pre-defined for both SHA-1 and SHA-256.
        
        M - message, a sequence of up to 55 bytes (440 bits) due to self-imposed 1 block limitation
            ASCII/hexadecimal constant is padded according to SHA1 specification
            random assignments are not padded
            the value is used for encoding, i.e.
            the formula is produced for the specified value originally, with maximal optimization
        H - hash value, a set of 160 bits for SHA-1 grouped into 5 32-bit words, 
            256 bits and 8 words for SHA-256 respectively
            the value is assigned afterwards with recursive UP and euqivalences optimizations
    
## Examples

1. Produce a generic SHA-1 CNF encoding without any message or hash value assigned.

        encode SHA1 sha1.cnf
            
2. Evaluate "sha1.cnf" with "CGen" ASCII string as a message padded for SHA-1, output hash value.  "sha1H.cnf" is empty since the formula is fully evaluated/satisfied.

        assign -vM string:CGen pad:sha1 sha1.cnf sha1H.cnf

3. Evaluate "sha1.cnf" with "CGen" ASCII string as a message padded for SHA-1. Then, compute hash value and assign it to "sha1.cnf". Store the result in "sha1H.cnf".

        assign -vM string:CGen pad:sha1 except:1..512 -vH compute sha1.cnf sha1H.cnf

4. Assign hash value to "sha1.cnf". Result is equivalent to the previous example.

        assign -vH 0x8dc19dbaebdceb30360c0e52c97dd6944e9889e9 sha1.cnf sha1H.cnf

5. Generate a SHA-1 encoding with hash value assigned. Result is equivalent to the previous example.
    
        encode SHA1 -vH 0x8dc19dbaebdceb30360c0e52c97dd6944e9889e9 sha1H.cnf

6. Generate SHA-1 encoding for the given ASCII string as a message with hash value computed and assigned, also with the first 8 bits of the message left as variables. Note that since the values are big-endian, the first 8 bits are the most significant bits of the first word of the message.

        encode SHA1 -vM string:CGen pad:sha1 except:1..8 -vH compute sha1.cnf

7. Generate a SHA-1 encoding with both message and hash value set to random values except for the random 8 bits of the message which are kept as variables.

        encode SHA1 -vM random:504 -vH random:160 sha1_random.cnf
        
8. Produce a generic SHA-1 ANF encoding  without any message or hash value assigned.

        encode SHA1 -fANF sha1.anf

## Verification
It is possible to verify the correctness of the resulting formula using the following techniques and the tool itself.

1. Generating a SHA encoding with a particular message outputs its hash value. This value can be verified using any alternative SHA implementation including those available online, e.g. <http://www.sha1-online.com>.
2. Assigning a message to a previously generated SHA encoding outputs its hash value; the encoding may be:
   - fully unassigned, with all message and hash value bits as binary variables
   - with some bits set beforehand using "assign" command
   - with full hash value set beforehand using "assign" command
     - assigning the correct message will output SATISFIABLE
     - assigning a conflicting message will result in a CONFLICT
        
### Example
A sample encoding is obtained using [SHA1-SAT](https://github.com/vegard/sha1-sat) with the following parameters:
    
    sha1-sat --cnf --tseitin-adders --message-bits=0 --hash-bits=0 > vn_original.cnf

CGen can be used to define the "M" and "H" variables. Equivalent definitions  can also be added manually to "vn_original.cnf" file as comments.

    cgen define -vM {{32/32/-1}/16/32} -vH {2752/32/-1}/5/32 vn_original.cnf vn_cgen.cnf
            
Subsequently, assigning a message ("CGen" ASCII string padded for SHA-1) yields its hash value.

    cgen assign -vM string:CGen pad:sha1 vn_cgen.cnf vn_evaluated.cnf

Additional variable assignments are possible. Note that variable numbers are native to the encoding and might change after optimizations.

    var W = {{32/32/-1}/16/32, {543/31/-1, 544}/64/32}
    var A = {2912/32/-1}/80/32

## PolyBoRi Output Format
CGen outputs an encoding in the below text format when ANF is chosen for output.

1. Any line that starts with the symbol "c" is considered to be a comment and can be ignored
2. The first few lines are comments which show:
    - number of variables and equations that form the polynominal system
    - encoding parameters (algorithm, number of rounds)
    - named variable definitions (see Named variables above)
3. Following the header comments, there is a list of equations, one equation  per line.
    - each equation is represented with an expression and should be interpreted as <expression> = 0
    - each equation is a set of terms; the terms are summed up (xor'ed) which is represented with symbol "+"
    - a term is one of:
        - a constant 1
        - a single variable
        - a product/conjunction of multiple variables represented with "*" symbol
        - each variable reference consists of "x" prefix followed by variable number; variable numbers start from 1
    - the tool will never output the same term twice within the same equation, duplicates are optimized out

## Acknowledgements & References
In many respects, CGen is an evolution of work done by other researchers. Below is the list of publications and tools used during CGen development.

1. Johanovic et al, 2005, <http://csl.sri.com/users/dejan/papers/jovanovic-hashsat-2005.pdf>
2. [Marjin Heule](http://www.cs.utexas.edu/users/marijn/), 2008, <https://repository.tudelft.nl/islandora/object/uuid%3Ad41522e3-690a-4eb7-a352-652d39d7ac81>
3. Norbert Manthey, 2011, <https://www.researchgate.net/publication/51934532_Coprocessor_-_a_Standalone_SAT_Preprocessor>
4. [Vegard Nossum](https://github.com/vegard), 2012, <https://www.duo.uio.no/handle/10852/34912>
5. Legendre et al., 2014, <https://eprint.iacr.org/2014/239.pdf>
6. Nejati et al., 2016, <https://www.researchgate.net/publication/306226194_Adaptive_Restart_and_CEGAR-based_Solver_for_Inverting_Cryptographic_Hash_Functions>
7. Motara, Irving, 2017, <https://researchspace.csir.co.za/dspace/bitstream/handle/10204/9692/Motara_19661_2017.pdf?sequence=1&isAllowed=y>
8. [Mate Soos](https://www.msoos.org) blog, <https://www.msoos.org>
9. [SHA1-SAT](https://github.com/vegard/sha1-sat) by Vegard Nossum
10. [Espresso Logic Minimizer](https://en.wikipedia.org/wiki/Espresso_heuristic_logic_minimizer) by Robert Brayton

ANF representation has been inroduced thanks to and based on advice received from [Mate Soos](https://github.com/msoos). Authors of [Bosphorus](https://github.com/meelgroup/bosphorus) used the resulting SHA-256 ANF encoding as one of benchmarks discussed in their [paper](https://www.comp.nus.edu.sg/~meel/Papers/date-cscm19.pdf).

CGen does not reuse any pre-existing source code.

## About
I developed CGen part of hobby research into SAT and cryptography.
If you have any questions about the tool, you can reach me at [vs@sophisticatedways.net](mailto:vs@sophisticatedways.net).
