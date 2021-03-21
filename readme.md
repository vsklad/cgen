# CGen
CGen is a tool for encoding [SHA-1](https://en.wikipedia.org/wiki/SHA-1) and [SHA-256](https://en.wikipedia.org/wiki/SHA-2) hash functions into [CNF](https://en.wikipedia.org/wiki/Conjunctive_normal_form) in [DIMACS](http://www.satcompetition.org/2009/format-benchmarks2009.html) format, also into [ANF](https://en.wikipedia.org/wiki/Algebraic_normal_form) polynominal system in [PolyBoRi](http://polybori.sourceforge.net) output format. The tool produces compact optimized encodings. The tool allows manipulating CNF encodings further via assignment of variable values and subsequent optimization.

- Project page: <https://cgen.sophisticatedways.net>.
- Article: <https://blog.sophisticatedways.net/2020/10/tailored-compact-cnf-encoding-for-sha-1.html>.
- Source code is published under [MIT license](https://github.com/vsklad/cgen/blob/master/LICENSE).
- Source code is available on GitHub: <https://github.com/vsklad/cgen>.

The current published version is 1.2. See [Change Log](#change-log) for details.

[![Build Status](https://travis-ci.org/vsklad/cgen.svg?branch=master)](https://travis-ci.org/vsklad/cgen)

## Description
CGen is built to make it easier to analyse [SAT problems](https://en.wikipedia.org/wiki/Boolean_satisfiability_problem) through manipulation of their CNF (DIMACS) and ANF (PolyBoRi) representations. SHA-1/SHA-2 (SHA) encodings are chosen as a reference/example implementation. The tool produces SHA encodings with different assignments of both message and hash values. Design of the tool is not limited to these algorithms.

Beyond encoding, CGen implements a set of CNF pre-processing techniques which are applied after assigning of variable values. The techniques include unit propagation, equivalences reasoning, limited binary clauses resolution and removal of subsumed clauses. This way, CGen is a simple SAT preprocessor similar to [SatELite](http://minisat.se/SatELite.html) and [Coprocessor](http://tools.computational-logic.org/content/riss.php). CGen does not implement any variant of [DPLL algorithm](https://en.wikipedia.org/wiki/DPLL_algorithm) and does not make any decisions with respect to variable values.

CGen implements three notable features.

### Compact encoding
The encoding is optimized to minimize both number of variables and number of clauses/equations. The assumption is that without redundancies, it may be easier to analyse the problem's complexity and structure. Application of the below techniques results in substantially more compact encodings than those published to date and known to the author. As an example, full SHA-1 is encoded into CNF with 26,156 variables and 127,200 clauses.

1. New variables are introduced only when necessary. For example, rotation, negation, operations where all arguments are constants, require no additional variables.
2. All boolean algebra operations/primitive functions are encoded in the most efficient possible way.
3. For CNF, every combination of bitwise n-nary addition is statically optimized using [Espresso Logic Minimizer](https://en.wikipedia.org/wiki/Espresso_heuristic_logic_minimizer), as a set of pre-defined clauses. This includes simplifications when one or more operands are constants.
4. For boolean algebra operations, any resultant constant values are optimized away during encoding. For example, (x ^ ~x ^ y) is  encoded as (y) without any new clauses or variables generated.

Optional simplification can be applied to any CNF formula including one encoded by the tool, particularly after assigning variable values:
1. Elimination of tautological and duplicated clauses
2. Unit propagation, equivalences reasoning and limited binary clauses resolution executed while assigning variables in an existing encoding
3. Removal of subsumed clauses
4. Elimination of gaps in variable numbering

### Named variables
A set of literals and constants can be grouped and given a name.  For example, hash value is a set of 160 literals and constants, depending on the encoding parameters. The value of such "named variable" may change throughout subsequent manipulations. CGen tracks these changes. Further, CGen looks for internal structure when a large number of binary variables is grouped together, to produce the most compact representation. For example, {1/32/1}/16/32 describes a set of 16 32-bit words with 512 variables corresponding sequentially to each bit. These descriptions are stored within the DIMACS file as comments of particular format.

In particular, named variables are specified as a combination of:

1. A 1- or 2-dimentional array of CNF literals and constants. Second dimention allows for mapping arrays of words (useful for SHA algorithms).
2. Sequences of literals and constants where diference (step) between elements is the same.
3. Sequences of binary constants which are grouped into binary and hexadecimal numbers.

For a SHA encoding the tool defines all variables referenced in the algorithm. Out of those M is for message and H is for the hash value.

### Assignment of parameters
CGen allows defining and assigning named variables via command line. For a new encoding, SHA message if specified, becomes embedded into it. In all other instances
the tool performs basic validation of the input, then applies the implemented pre-processing techniques to optimise the encoding. Alternatively, it is possible to define named variables by editing the DIMACS file manually.

In particular, CGen supports:

1. Use of binary, hexadecimal and character-sequence constants (with bits mapped to binary variables).
2. Setting of specific bits/binary variables of the named variable.
3. Assigning random values to a subset of randomly chosen binary variables.
4. Padding of 1-block SHA messages.
5. Computing hash value given a constant message, then assigning it along with some (or without) bits of the message fixed.

Furthermore, it is possible to combine or sequence multiple assignments of the same named variable within the same encoding, running the tool several times with different parameters and analysing the results.

## Usage

### Build
Source code is hosted at GitHub. To obtain and build:

    git clone https://github.com/vsklad/cgen
    make
    
CGen has no external dependencies other than [C++ STL](https://en.wikipedia.org/wiki/Standard_Template_Library). [C++ 11](https://en.wikipedia.org/wiki/C%2B%2B11) is a requirement. The makefile relies on [GNU g++](https://gcc.gnu.org/) compiler. Verified with GCC 4.8.4, GCC 4.9.2 and Make 3.81.

### Run
Launch "./cgen" for OSX/Linux or "cgen.exe" for Windows.

### Command Line
The tool supports the following parameters:

    cgen encode (SHA1|SHA256) [-r <rounds>] [-v <name> <value>]... [<encoder_options>] [<output_options>] [<output_file_name>]
    cgen process [<variable>]... [<output_options>] <input_file_name> [<output_file_name>]
    cgen --help
    cgen --version

#### Commands

        encode (SHA1|SHA256) - generate the encoding and 
            save it in the chosen format (-f option) to <output_file_name>;
            can generate the encoding for specified number of rounds (-r option);
            also, assign the message and hash values fully or partially (-v option);
            if <output_file_name> is omitted, a default file name is assumed

        process - process and existing CNF/DIMACS or ANF file
            read the formula from <input_file_name>,
            assign existing variables if specified then simplify the formula and save it to <output_file_name>;
            for an existing variable, its spefification is updated or replaced (see -v replace);
            otherwise if the specified variable does not exist, its definition is added;
            this command accepts any CNF/DIMACS files, not necessarily produced by the tool;
            if <output_file_name> is omitted, the resulting CNF is not saved;
                this can be used to compute values of named variables
            
#### Options

        <variable> = -v <variable_name> <variable_value> | -v<variable_name>=<variable_value>

            specification of a named variable (template) or a single binary variable 
            named variable is a sequence of literals and constants represented with bits
            its template maps to binary variables and/or constant values
            named variable definitions are recorded in custom "var" statements 
            in comments within the output (e.g. DIMACS) file
            
            <variable_name> is the name of the variable, case sensitive
                if a decimal number, treated as a binary variable number
                  must be a valid binary variable number from the formula
                otherwise, treated as a named variable name
                  may contain characters '0'..'1', 'a'..'z', 'A'..'Z', '_'
                  with first symbol may not a digit
                an option with the particular variable may only appear once
            <variable_value> is the variable value or specification
                constant binary variables may be grouped into bytes
                and further represented as a string, a hexadecinal or binary number
                sequential representation can be further optimized as below
                if a value includes spaces, it must surrounded with double-quotes
                to allow the shell process it as a single argument
                
            supported specifications:
                <value> = ( <value_data> | random | ( compute [<compute_options>] )) [<except>]
                <value_data> = ( <value_template> | <str_value> ) [<pad>] [ replace ]
                
                <value_template> = (<literal> | <hex_value> | <bin_value> | <non_value>)[<sequence_clause>]
                <value_template> = "{" <variable_template> ["," <variable_template>]..."}"[<sequence_clause>] 
                    defines a named variable by describing a set of literals and constants
                <sequence_clause> = "/"<count>["/"<step>]
                    defines a sequence of <count> elements where the first item is the specified value
                    each subsequent element is produced by adding <step> to the previous one
                    <step> other than zero is allowed for variable sequences and nested elements only
                    <step> can be negative but the sequence is limited to valid variable numbers
                    for nested elements
                        the same <step> applies to every variable within the element
                        constant and unassigned values are the same for each element in the sequence

                <hex_value> = 0x['0'..'9', 'a'..'f', 'A'..'F']...
                    a big endian hexadecimal constant
                    each hexadecimal digit is mapped to 4 binary variables
                <bin_value> = 0b['0'..'1']...
                    a big endian binary constant
                    each symbol is mapped to 1 binary variable
                <str_value> = string:[\0x21..\0x7F]... | "[\0x20..\0x7F]..."
                    an ASCII string, with quotes if includes one or more space symbols
                    big endian format is assumed if mapped to word(s)
                    each character is mapped to 8 binary variables
                <non_value> = *
                    for existing named variables, the value should be kept as in the definition
                    otherwise, means that the value is unassigned and can be anything

                random
                    assigns binary variable or binary variables (bits) of the named variable 
                      to random values (uniform distribution);
                    constant binary values remain unchanged;
                    <except_clause> can be used to control partial assignment,
                    i.e. to assign certain number of bits or exclude certain range from assigning;
                    the named variable must be defined (exist) before the assignment
                    
                compute
                    the value is computed and the formula is processed as follows:
                    1. assign other variables to the specified values without <except_clause> applied
                    2. determine the computed value by encoding/evaluating the formula with those assignments
                    3. encode the formula with all variables and <except_clause> taken into account
                    the named variable must be defined (exist) in order to compute its value
                    
                <compute_options> = complete | difference | constant
                    these options define the composition of the computed value
                    partial representations without variables can be used to map values across formulas
                        "complete" - all bits are assigned either from variable template or as calculated;
                            when the value is assigned, some binary variables may be assigned to themselves;
                            this has no effect on the result however
                        "difference" - only those bits that are different from the template, are assigned
                            bits with values equal to those in the template, are unasssigned
                        "constant" - the value includes all constant binary values but no variables
                            all variable bits are unassigned
                    "difference" is the default value
                    
                <pad> = pad:(SHA1 | SHA256)
                    the variable treated as a SHA-1/SHA-256 message and is padded accordingly;
                    not supported for assigning of individual binary variables
                    not supported for "random" and "compute" modes
                    
                <except> = except:(( <first>..<last> ) | <count> )
                    <first>..<last> - range of binary positions/variables to prevent from assigning
                       one-based index, i.e. <first> and <last> must be between 1 and the size of the variable;
                    <count> - choose the requested number of bits randomly (uniform distribution)
                       and prevent them from being assigned;
                       the number of bits is chosen from those set to constants within the specified value
                       where the underlying variable has them non-constant
                    the named variable must be defined previously
                    not supported for assigning of individual binary variables
                    
                replace
                    if the named variable exists, forces its redefinition in the DIMACS file
                    the existing definition is ignored
                    not supported for assigning of individual binary variables
                    
                Notes:
                    named variable is interpreted as new if it does not exist or if "replace" option is specified;
                    otherwise, the value is assigned to the prevously defined variable before processing the formula;
                    the non-constant bits of the named variable are set to constant bits from the value;
                    in all other instances the named variable bits and the value bits must match
        
        -r <value>
            number of algorithm rounds to encode
            the value must be valid for a particular algorithm (e.g. 1 to 80 for SHA-1)
            if unspecified, the algorithm is encoded in full with all rounds
            this option is only valid for <encode> command
            
        -h | --help
            output parameters/usage specification
            
        --version
            output version number of the tool
        
        <encoder options>
            [--add_max_args=<value>] [--xor_max_args=<value>]
                these options define how the encoder processes addition and xor's for multiple operands
                these options are allowed for CNF only (not allowed for ANF)
                specify maximal number of binary variables to be encoded together, plus the result
                any constants are optimized out when encoding and are outside this number 
                <add_max_args> is a number between 2 and 6 for CNF, 3 for ANF, 3 if not specified;
                    defines maximal number of binary variables to be added together
                <xor_max_args> is a number between 2 and 10, 3 if not specified
                    defines maximal number of binary variables to be xor'ed together
                    
            [--assign_after_encoding]
                assign all variable values if any specified, after encoding;
                ignore those values while encoding;
                similar to "unoptimized" pre-processing mode in relation to encoding;
                i.e. no formula optimization while encoding
                i.e. produce SHA-1 CNF formula with 512 variable message bits
                    then assign provided message bit values according to -m processing option;
                if this option is not specified,
                    the provided variable values are used to encode a simple formula if possible
                
        <output options>
            -f<output_format_name>
                output type/form/format, CNF if not specified;
                the following formats are supported:
                    ANF - output ANF formula in PolyBoRi output format
                    (CNF | DIMACS_CNF) - output CNF formula in DIMACS format
                    (VIG | VIG_GraphML) - output VIG^ obtained from CNF formula, in GraphML format
                    (VIGW | VIGW_GraphML) - output VIG obtained from CNF formula, with weighted edges 
                    (VIG_GEXF) - output VIG obtained from CNF formula, in GEXF format
                ^VIG stands for Variable Incidence Graph
            
            -m ((unoptimized | u) | (all | a) | (original | o))
                mode for pre-processing and assigning of variable values
                has no effect on how the formula is encoded if used with <encode>;
                (unoptimized | u)
                    no simplification techniques; assign variable values by 
                    appending unary clauses or equations to the output formula;
                    this is the only supported and default mode for ANF
                (original | o)
                    apply simplification techniques; eliminate variables;
                    output included only clauses present in the formula 
                    originally after encoding;
                    determined variable values are propagated
                    simplifying and eliminating some of the clauses
                (all | a)
                    apply pre-processing techniques; eliminate variables;
                    output includes the original clauses plus all clauses 
                    produced by applying resolution rule while pre-processing;
                    determined variable values are propagated
                    simplifying and eliminating some of the clauses
                    
            --no_variable_reindexing
                do not reindex binary variable numbers after processing;
                if this option is not specified the tool would reindex 
                variable numbers and remove any gaps in numbering;
                this option is only applicable following simplification

            -n | --normalize_variables
                reindex binary variables such that no negations are present 
                within named variable definitions within the output file;
                may add aditional clauses/equations as necessary
                supported for encode/process commands
                
            (-t | --trace)((debug | d) | gexf | (native | n))
                record and output a trace of the formula simplification process;
                applicable to all output formats based on CNF;
                available only when compiled with CNF_TRACE directive;
                available formats:
                debug | d
                  print the trace in "native" format
                gexf
                  the trace is recorded as a dynamic graph in a file with 
                  name obtained by adding ".trace.gexf" to output_file_name
                native | n
                  the trace is recorded in a file in "native" format, its name
                  being the output_file_name with ".trace" expension appended

### Pre-defined Variables
Two variables are pre-defined for both SHA-1 and SHA-256.
        
        M - message, a sequence of up to 55 bytes (440 bits) due to self-imposed 1 block limitation
            ASCII/hexadecimal constant is padded according to SHA1 specification
            note: random assignments are not padded
            the value is used for encoding, i.e.
            the formula is produced for the specified value originally, with maximal optimization
        H - hash value, a set of 160 bits for SHA-1 grouped into 5 32-bit words, 
            256 bits and 8 words for SHA-256 respectively
            the value is assigned afterwards with recursive UP and euqivalences optimizations
    
## Examples

1. Produce a generic SHA-1 CNF encoding without any message or hash value assigned.

        encode SHA1 sha1.cnf
            
2. Evaluate "sha1.cnf" with "CGen" ASCII string as a message padded for SHA-1, output hash value.  "sha1H.cnf" is empty since the formula is fully evaluated/satisfied.

        process -vM string:CGen pad:sha1 sha1.cnf sha1H.cnf

3. Evaluate "sha1.cnf" with "CGen" ASCII string as a message padded for SHA-1. Then, compute hash value and assign it to "sha1.cnf". Store the result in "sha1H.cnf".

        process -vM string:CGen pad:sha1 except:1..512 -vH compute sha1.cnf sha1H.cnf

4. Assign hash value to "sha1.cnf". Result is equivalent to the previous example.

        process -vH 0x8dc19dbaebdceb30360c0e52c97dd6944e9889e9 sha1.cnf sha1H.cnf

5. Generate a SHA-1 encoding with hash value assigned. Result is equivalent to the previous example.
    
        encode SHA1 -vH 0x8dc19dbaebdceb30360c0e52c97dd6944e9889e9 sha1H.cnf

6. Generate SHA-1 encoding for the given ASCII string as a message with hash value computed and assigned, also with the first 8 bits of the message left as variables. Note that since the values are big-endian, the first 8 bits are the most significant bits of the first word of the message.

        encode SHA1 -vM string:CGen pad:sha1 except:1..8 -vH compute sha1.cnf

7. Generate a SHA-1 encoding with both message and hash value set to random values except for the random 8 bits of the message which are kept as variables.

        encode SHA1 -vM random except:8 -vH random sha1_random.cnf
        
8. Produce a generic SHA-1 ANF encoding  without any message or hash value assigned.

        encode SHA1 -fANF sha1.anf

## Verification
It is possible to verify the correctness of the resulting formula using the following techniques and the tool itself.

1. Generating a SHA encoding with a particular message outputs its hash value. This value can be verified using any alternative SHA implementation including those available online, e.g. <http://www.sha1-online.com>.
2. Assigning a message to a previously generated SHA encoding outputs its hash value; the encoding may be:
   - fully unassigned, with all message and hash value bits as binary variables
   - with some bits set beforehand using <process> command
   - with full hash value set beforehand using <process> command
     - assigning the correct message will output SATISFIABLE
     - assigning a conflicting message will result in a CONFLICT
        
### Example
A sample encoding is obtained using [SHA1-SAT](https://github.com/vegard/sha1-sat) with the following parameters:
    
    sha1-sat --cnf --tseitin-adders --message-bits=0 --hash-bits=0 > vn_original.cnf

CGen can be used to define the "M" and "H" variables. Equivalent definitions  can also be added manually to "vn_original.cnf" file as comments.

    cgen process -vM {{32/32/-1}/16/32} -vH {2752/32/-1}/5/32 vn_original.cnf vn_cgen.cnf
            
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

1. Jovanovic et al, 2005, <http://csl.sri.com/users/dejan/papers/jovanovic-hashsat-2005.pdf>
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

## Change Log
### Version 1.2
Version 1.2 includes a set of incremental improvements. The encoded raw CNF formula is unchanged.
Key changes:
- Various options added for specifying variables values / constraints (e.g. SHA message and hash values). 
- Variable assignments supported for ANF.
- Optional CNF simplification expanded (e.g. to elimilate subsumed clauses). 
- Graph output suported essentially incorporating CGraph functionality.
- Can produce detailed trace of all simplification actions/steps while processing the formula (-t option).
### Version 1.1
- Original version published in 2018

## About
I developed CGen part of hobby research into SAT and cryptography.
If you have any questions about the tool, you can reach me at [vs@sophisticatedways.net](mailto:vs@sophisticatedways.net).
