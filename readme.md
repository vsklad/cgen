# CNFGen
CNFGen is a tool for encoding SHA-1 and SHA-256 hash functions into CNF in DIMACS format.

The tool generates compact optimized [DIMACS](http://www.satcompetition.org/2009/format-benchmarks2009.html) encodings of SHA1 hash function.

Project page: <https://cnfgen.sophisticatedways.net>
Source code is published under MIT license.
Source code is available on GitHub: <https://github.com/vsklad/cnfgen>

## Description
CNFGen is built to make it easier to analyse CNF SAT problems through manipulation of their DIMACS representation. SHA-1/SHA-2 (SHA) encodings are chosen as a reference/example implementation. The tool generates DIMACS files for SHA algorithms with different assignments of both message and hash values. Design of the tool is not limited to these algorithms.

Beyond encoding techniques, CNFGen implements recursive unit propagation and elimination of equivalences. This way, it is a simple SAT preprocessor similar to [SatELite](http://minisat.se/SatELite.html) and [Coprocessor](http://tools.computational-logic.org/content/riss.php). At the  same time, CNFGen does not implement any variant of [DPLL algorithm](https://en.wikipedia.org/wiki/DPLL_algorithm) and does not guess any variable values.

CNFGen implements three notable features.

### Compact encoding
The encoding is optimized to minimize both number of variables and number of clauses. The assumption is that with redundancies removed, it may be easier to assess the problem's complexity and structure. Application of the below techniques results in substantially more compact encodings than those published to date and known to the author.

1. new variables are introduced only when necessary during encoding, e.g. no additional variables are needed for rotation and negation
2. all logical operations/primitives are encoded in the most efficient possible way
3. every combination of bitwise n-nary addition is statically optimized using [Espresso Logic Minimizer](https://en.wikipedia.org/wiki/Espresso_heuristic_logic_minimizer), as a set of pre-defined clauses. This includes simlifications when one or more operands are constants
4. any resultant constant values are optimized away during encoding; e.g. x ^ ~x is  optimized as 1
5. recursive unit propagation and elimination of equivalences while assigning variables for an existing encoding

### Named variables
A set of literals and constants can be grouped and given a name.  For example, a hash value is a set of 160 literals and constants, depending on encoding parameters.
Such "named variable" may change throughout subsequent manipilations. CNFGen tracks these changes. Further, CNFGen looks for internal structure when a large number of binary variables is grouped together, to produce the most compact representation. For example, {1/32/1}/16/32 describes a set of 16 32-bit words with 512 variables corresponding sequentially to each bit. These descriptions are stored within the DIMACS file as specially formatted comments.
For a SHA encoding, at least two variables are defined, M for message and H for the hash value.

CNFGen allows specifying named variable definitions and values via command line. While is possible to do the same by editing the DIMACS file, CNFGen perform basic valudation of the input. In particular, CNFGen supports:

1. use of binary, hexadecimal and character-sequence constants (with bits mapped to binary variables) 
2. setting of specific bits/binary variables of the named variable
3. specifying random binary values
4. padding 1-block SHA messages

### Assignment of parameters
CNFGen supports three modes of setting SHA parameters. It is possible to assign incrementally, i.e. assign more binary variables in the same DIMACS file each time, analysing the impact.

1. setting a number of randomly chosen variables to random values
2. setting some or all bits/bytes of the message and/or hash value to pre-defined constants
3. computing hash value given a constant message, then assigning it along with partially assigned messge

## Usage

### Build
Source code is hosted at GitHub. To obtain and build:

    git clone https://github.com/vsklad/cnfgen
    make
    
### Run
To run the tool please launch "./cnfgen" for OSX/Linux and "cnfgen.exe" for Windows.

### Command Line
The tool supports the following parameters:

    cnfgen encode (SHA1|SHA256) [-r <rounds>] [-v <name> <value>]... [<encoder options>] <output file name>
    cnfgen (assign | define) [-v <name> <value>]... <input file name> <output file name>
    cnfgen --help
    cnfgen --version

    Commands
        encode (SHA1|SHA256) - generate the encoding and 
            save it in DIMACS format to the specified <output file name>
            can generate the encoding for specified number of rounds (-r option),
            also partialy or fully assign the message and hash values (-v option)
        assign - read <input file name>, assign variables as specified and save it to <output file name>
            requires input file in DIMACS format produced by the same tool
            this is due to variable specifications encoded as DIMACS comments
            these variable specificatiosn may be added to an externally produced file
            using the <define> command
        define - define one or more named variables
            accepts any DIMACS files, including those produced by other means
            records variable definitions in the output file as DIMACS comments
            "random", "compute" and "except" options are not supported within variable definitions
            
    Options
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
                
        -r <value>
            number of SHA1 rounds to encode
            the value mut be between 1 and 80, defaults to 80 if not specified
            
        -h | --help
            output parameters/usage specification
            
        --version
            output version number of the tool
            
        <encoder options> [--add_max_args=<value>] [--xor_max_args=<value>]
            these options define how the encoder processes addition and xor's for multiple operands
            maximal number of binary variables to be encoded together, plus the result
            any constants are optimized out when encoding and are outside this number 
            <add_max_args> is a number between 2 and 6, 3 if not specified
                defines maximal number of binary variables to be added together
            <xor_max_args> is a number between 2 and 10, 3 if not specified
                defines maximal number of binary variables to be xor'ed together
        
### Pre-defined Variables
Two variables are pre-defined for SHA-1 and SHA-256.
        
        M - message, a sequence of up to 55 bytes (440 bits) due to self-imposed 1 block limitation
            ASCII/hexadecimal constant is padded according to SHA1 specification
            random assignments are not padded
            the value is used for encoding, i.e.
            the formula is produced for the specified value originally, with maximal optimization
        H - hash value, a set of 160 bits for SHA-1 grouped into 5 32-bit words, 
            256 bits and 8 words for SHA-256 respectively
            the value is assigned after with UIP and euqivalences optimizations
    
## Examples

1. Generate a generic SHA1 encoding without any message or hash value assigned

        encode SHA1 sha1.cnf
            
2. Evaluate <sha1.cnf> with "CNFGen" ASCII string as a message, output hash value. 
    <sha1H.cnf> is empty since the formula is fully evaluated/satisfied

        assign -vM string:CNFGen pad:sha1 sha1.cnf sha1H.cnf

3. Evaluate sha1.cnf with "CNFGen" ASCII string as a message, 
    compute hash value then assign it to <sha1.cnf> and store the result to sha1H.cnf

        assign -vM string:CNFGen pad:sha1 except:1..512 -vH compute sha1.cnf sha1H.cnf

4. Assign hash value to sha1.cnf, result is equivalent to the previous example

        assign -vH 0xa11f66f0e618011e6cfab657cb05c8fa7c23bc26 sha1.cnf sha1H.cnf 

5. Generate a SHA1 encoding with hash value assigned. 
    Result is equivalent to the previous example.
    
        encode SHA1 -vH 0xa11f66f0e618011e6cfab657cb05c8fa7c23bc26 sha1H.cnf

6. Generate SHA1 encoding for the given ASCII string as a message
    with hash value computed and assigned
    and with the first 8 bits of the message left as variables.

        encode SHA1 -vM string:CNFGen except:1..8 -vH compute sha1.cnf

7. Generate SHA1 encoding with both message and hash value set to random values
    except for the random 8 bits of the message are kept as variables

        encode SHA1 -vM random:504 -vH random:160 sha1_random.cnf

## Verification

It is possible to verify the correctness of the resulting formula using the following techniques and the tool itself:
1. generating SHA1 encoding with a particular message outputs its hash value
            this value can be verified using any alternative SHA1 implementation
            including those available online, e.g. http://www.sha1-online.com/
2. assigning a message to a previously generated SHA1 encoding outputs its hash value; the encoding may be:
   - fully unassigned, with all message and hash value bits as binary variables
   - with some bits set beforehand using "assign" command
   - with full hash value set beforehand using "assign" command
     - assigning the correct message will output SATISFIABLE
     - assigning a conflicting message will result in a CONFLICT
        
### Example
A sample encoding is obtained using SHA1-SAT (https://github.com/vegard/sha1-sat) with the following parameters:
    
    sha1-sat --cnf --tseitin-adders --message-bits=0 --hash-bits=0 > vn_original.cnf

CNFGen can be used to define . Equivalent definitions  can also be added manually to <vn_original.cnf> as comments.

    cnfgen define -vM {{32/32/-1}/16/32} -vH {2752/32/-1}/5/32 vn_original.cnf vn_cnfgen.cnf
            
Subsequently, assigning a message ("CNFGen" padded for SHA-1) yields its hash value.

    cnfgen assign -vM string:CNFGen pad:sha1 vn_cnfgen.cnf vn_evaluated.cnf

Other possible variable assignments, note that variable numbers are original to the encoding, might change after optimizations:

    var W = {{32/32/-1}/16/32, {543/31/-1, 544}/64/32}
    var A = {2912/32/-1}/80/32

## Acknowledgements & References
CNFGen is partly inspired by earlier work done by several researchers and builds on it.

1. Johanovic et al, 2005, http://csl.sri.com/users/dejan/papers/jovanovic-hashsat-2005.pdf
2. Marjin Heule, 2008, https://repository.tudelft.nl/islandora/object/uuid%3Ad41522e3-690a-4eb7-a352-652d39d7ac81
3. Norbert Manthey, 2011, https://www.researchgate.net/publication/51934532_Coprocessor_-_a_Standalone_SAT_Preprocessor
4. Vegard Nossum, 2012, https://www.duo.uio.no/handle/10852/34912
5. Legendre et al., 2014, https://eprint.iacr.org/2014/239.pdf
6. Nejati et al., 2016, https://www.researchgate.net/publication/306226194_Adaptive_Restart_and_CEGAR-based_Solver_for_Inverting_Cryptographic_Hash_Functions
7. Motara, Irving, 2017, https://researchspace.csir.co.za/dspace/bitstream/handle/10204/9692/Motara_19661_2017.pdf?sequence=1&isAllowed=y
8. Robert Brayton, Espresso Logic Minimizer, https://en.wikipedia.org/wiki/Espresso_heuristic_logic_minimizer

CNFGen does not reuse any pre-existing source code.
The encodings are substantially more compact as measured by number of variables and clauses, than other published SHA-1 and SHA-256 encodings known to the author.

## About
I developed this tool part of hobby reserach into SAT and cryptography.

