Included are 4 source files: Transformer1, Transformer2, Transformer3, and magical_transformer.

COMPILATION INSTRUCTIONS (magical transformer expects these specific object file names):
    gcc Transformer1.c -o Transformer1
    gcc Transformer2.c -o Transformer2
    gcc Transformer3.c -o Transformer3
    gcc magical_transformer.c -o magical_transformer

Transformers 1-3 can be executed individually and all expect input as described in the assignment 1 documentation.

Transformer1 takes raw input data. Assuming the testcases folder is in the same directory as the transformer, it can be run with:

    ./Transformer1 <input.txt

Transformer2 takes performance data. Assuming the testcases folder is in the same directory as the transformer, it can be run with:

    ./Transformer2 <performance.txt

Transformer3 takes rating data. Assuming the testcases folder is in the same directory as the transformer, it can be run with:

    ./Transformer3 <rating.txt

Magical transformer takes raw input data and creates 3 child processes to run Transformers 1-3. It expects command line arguments as outlined in the doc.

    ./magical_transformer <input.txt stderr:agent_performance stdout:state_rating 1>stdout.txt 2>stderr.txt 

