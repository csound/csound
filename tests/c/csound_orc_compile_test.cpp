/*
 * File:   main.c
 * Author: stevenyi
 *
 * Created on June 7, 2012, 4:03 PM
 */

#define __BUILDING_LIBCSOUND


#include <stdio.h>
#include <stdlib.h>
#include "csoundCore.h"
#include "gtest/gtest.h"

#define csoundCompileOrc(a,b) csoundCompileOrc(a,b,0)
#define csoundReadScore(a,b) csoundEventString(a,b,0)

class OrcCompileTests : public ::testing::Test {
public:
    OrcCompileTests ()
    {
    }

    virtual ~OrcCompileTests ()
    {
    }

    virtual void SetUp ()
    {
        csound = csoundCreate (NULL,NULL);
        //csoundCreateMessageBuffer (csound, 0);
        csoundSetOption (csound, "-odac --logfile=null");
    }

    virtual void TearDown ()
    {
        csoundDestroy (csound);
        csound = nullptr;
    }

    CSOUND* csound {nullptr};
};

extern "C" {
    extern int32_t args_required (const char* arrayName);
    extern char** split_args (CSOUND* csound, const char* argString);
}

TEST_F (OrcCompileTests, testArgsRequired)
{
    ASSERT_EQ (1, args_required("a"));
    ASSERT_EQ (2, args_required("ka"));
    ASSERT_EQ (3, args_required("kak"));
    ASSERT_EQ (2, args_required("ak"));
    ASSERT_EQ (3, args_required("a[]ka"));
    ASSERT_EQ (4, args_required("a[]k[]ka"));
    ASSERT_EQ (4, args_required("a[][]k[][]ka"));
    ASSERT_EQ (0, args_required(NULL));
}

TEST_F (OrcCompileTests, testSplitArgs)
{
    char** results = split_args(csound, "kak");

    ASSERT_STREQ ("k", results[0]);
    ASSERT_STREQ ("a", results[1]);
    ASSERT_STREQ ("k", results[2]);
    csound->Free(csound, results);

    results = split_args(csound, "a[]k[]ka");

    ASSERT_STREQ ("[a]", results[0]);
    ASSERT_STREQ ("[k]", results[1]);
    ASSERT_STREQ ("k", results[2]);
    ASSERT_STREQ ("a", results[3]);
    csound->Free(csound, results);

    results = split_args(csound, "a[][]k[][]ka");

    ASSERT_STREQ ("[[a]", results[0]);
    ASSERT_STREQ ("[[k]", results[1]);
    ASSERT_STREQ ("k", results[2]);
    ASSERT_STREQ ("a", results[3]);
    csound->Free(csound, results);
}

TEST_F (OrcCompileTests, testCompile)
{
    int32_t result, compile_again = 0;
    const char* instrument =
        "instr 1 \n"
        "k1 expon p4, p3, p4*0.001 \n"
        "a1 randi  k1, p5   \n"
        "out  a1   \n"
        "endin \n";

    const char* instrument2 =
        "instr 2 \n"
        "k1 expon p4, p3, p4*0.001 \n"
        "a1 vco2  k1, p5   \n"
        "out  a1   \n"
        "endin \n"
        "event_i \"i\",2, 0.5, 2, 10000, 800 \n";

    result = csoundCompileOrc(csound, instrument);
    ASSERT_TRUE (result == 0);
    csoundReadScore(csound,  "i 1 0  1 10000 5000\n i 1 3 1 10000 1000\n");
    result = csoundStart(csound);
    ASSERT_TRUE (result == 0);

    while(!result)
    {
        result = csoundPerformKsmps(csound);

        if(!compile_again)
        {
            /* new compilation */
            csoundCompileOrc(csound, instrument2);
            /* schedule an event on instr2 */
            csoundReadScore(csound, "i2 1 1 10000 110 \n i2 + 1 1000 660");
            compile_again = 1;
        }
    }
}

TEST_F (OrcCompileTests, testNestedExpressionFailurePropagates)
{
    const char *instrument =
        "instr 1\n"
        "  iresult = abs(missing[0] + 1)\n"
        "endin\n";

    ASSERT_NE(CSOUND_SUCCESS, csoundCompileOrc(csound, instrument));
}

TEST_F (OrcCompileTests, testReuse)
{
    int32_t result;
    const char* instrument =
        "instr 1 \n"
        "k1 expon p4, p3, p4*0.001 \n"
        "a1 randi  k1, p5   \n"
        "out  a1   \n"
        "endin \n";

    result = csoundCompileOrc(csound, instrument);
    ASSERT_TRUE(result == 0);
    csoundReadScore(csound,  "i 1 0  1 10000 5000\n");
    result = csoundStart(csound);
    ASSERT_TRUE(result == 0);
    while(csoundPerformKsmps(csound) == 0);
    csoundReset(csound);
    result = csoundCompileOrc(csound, instrument);
    csoundRewindScore(csound);
    csoundReadScore(csound,  "i 1 0  1 10000 5000\n");

    while(csoundPerformKsmps(csound) == 0);
    csoundReset(csound);
    result = csoundCompileOrc(csound, instrument);
    csoundRewindScore(csound);
    csoundReadScore(csound,  "i 1 0  1 10000 5000\n i 1 3 1 10000 1000\n");

    while(csoundPerformKsmps(csound) == 0);
}

TEST_F (OrcCompileTests, testLineNumber)
{
    const char* instrument =
        "instr 1 \n"
        "k1 expon p4, p3, p4*0.001 \n"
        "a1 randi  k1, p5   \n"
        "out  a1   \n"
        "endin \n";

    TREE *tree = csoundParseOrc(csound, instrument);
    ASSERT_TRUE(tree != NULL);
}
#if 0
// Helper to dump AST tree for debugging column number tests
static void dump_tree(TREE *t, int depth) {
    if (!t) return;
    for (int i = 0; i < depth; i++) printf("  ");
    printf("type=%d line=%d", t->type, t->line);
    if (t->value) {
        printf(" lexeme=\"%s\" cols=%u-%u",
               t->value->lexeme ? t->value->lexeme : "(null)",
               t->value->first_column, t->value->last_column);
    }
    printf("\n");
    if (t->left) {
        for (int i = 0; i < depth; i++) printf("  ");
        printf(" LEFT:\n");
        dump_tree(t->left, depth + 1);
    }
    if (t->right) {
        for (int i = 0; i < depth; i++) printf("  ");
        printf(" RIGHT:\n");
        dump_tree(t->right, depth + 1);
    }
    if (t->next) {
        dump_tree(t->next, depth);
    }
}
#endif

// Test basic column number tracking on tokens
TEST_F (OrcCompileTests, testColumnNumbers)
{
    //          col: 123456789
    const char *orc = "instr 1\n"        // line 1: "1" at col 7
                      "k1 = 440\n"       // line 2: "k1" cols 1-2, "440" cols 6-8
                      "endin\n";

    TREE *tree = csoundParseOrc(csound, orc);
    ASSERT_TRUE(tree != NULL);

    // csoundParseOrc returns a root node (type=0);
    // the instr block is linked via ->next
    TREE *instr = tree->next;
    ASSERT_TRUE(instr != NULL);

    // LEFT child: instrument number "1"
    ASSERT_TRUE(instr->left != NULL);
    ASSERT_TRUE(instr->left->value != NULL);
    ASSERT_STREQ("1", instr->left->value->lexeme);
    ASSERT_EQ(7u, instr->left->value->first_column);
    ASSERT_EQ(7u, instr->left->value->last_column);

    // RIGHT child: the body "k1 = 440"
    TREE *body = instr->right;
    ASSERT_TRUE(body != NULL);

    // body LEFT: "k1" identifier
    ASSERT_TRUE(body->left != NULL);
    ASSERT_TRUE(body->left->value != NULL);
    ASSERT_STREQ("k1", body->left->value->lexeme);
    ASSERT_EQ(1u, body->left->value->first_column);
    ASSERT_EQ(2u, body->left->value->last_column);

    // body RIGHT: "440" number
    ASSERT_TRUE(body->right != NULL);
    ASSERT_TRUE(body->right->value != NULL);
    ASSERT_STREQ("440", body->right->value->lexeme);
    ASSERT_EQ(6u, body->right->value->first_column);
    ASSERT_EQ(8u, body->right->value->last_column);

    csoundDeleteTree(csound, tree);
}

// Test that column numbers reset correctly across multiple lines
TEST_F (OrcCompileTests, testColumnNumbersMultiLine)
{
    //          col: 123456789012345
    const char *orc = "instr 1\n"
                      "k1 = 100\n"       // line 2: "k1" cols 1-2
                      "k2 = 200\n"       // line 3: "k2" cols 1-2 (reset from prev line)
                      "endin\n";

    TREE *tree = csoundParseOrc(csound, orc);
    ASSERT_TRUE(tree != NULL);

    TREE *instr = tree->next;
    ASSERT_TRUE(instr != NULL);
    ASSERT_TRUE(instr->right != NULL);

    // First statement: k1
    TREE *stmt1 = instr->right;
    ASSERT_TRUE(stmt1->left != NULL && stmt1->left->value != NULL);
    ASSERT_STREQ("k1", stmt1->left->value->lexeme);
    ASSERT_EQ(1u, stmt1->left->value->first_column);
    ASSERT_EQ(2u, stmt1->left->value->last_column);

    // Second statement: k2 (via stmt1->next)
    TREE *stmt2 = stmt1->next;
    ASSERT_TRUE(stmt2 != NULL);
    ASSERT_TRUE(stmt2->left != NULL && stmt2->left->value != NULL);
    ASSERT_STREQ("k2", stmt2->left->value->lexeme);
    ASSERT_EQ(1u, stmt2->left->value->first_column);
    ASSERT_EQ(2u, stmt2->left->value->last_column);

    // Verify the values too
    ASSERT_TRUE(stmt1->right != NULL && stmt1->right->value != NULL);
    ASSERT_STREQ("100", stmt1->right->value->lexeme);
    ASSERT_EQ(6u, stmt1->right->value->first_column);
    ASSERT_EQ(8u, stmt1->right->value->last_column);

    ASSERT_TRUE(stmt2->right != NULL && stmt2->right->value != NULL);
    ASSERT_STREQ("200", stmt2->right->value->lexeme);
    ASSERT_EQ(6u, stmt2->right->value->first_column);
    ASSERT_EQ(8u, stmt2->right->value->last_column);

    csoundDeleteTree(csound, tree);
}

// Test column tracking through xstr ({{ }}) multi-line strings
// BUG: <xstr> newline rule does not reset yycolumn, so the column
// reported for the STRING_TOKEN (at the closing }}) is wrong.
TEST_F (OrcCompileTests, testColumnNumbersXstr)
{
    const char *orc = "instr 1\n"                 // line 1
                      "Sval = {{\n"                // line 2: Sval col 1-4
                      "hello\n"                    // line 3: inside string
                      "}}\n"                       // line 4: }} at cols 1-2
                      "k1 = 440\n"                 // line 5
                      "endin\n";

    TREE *tree = csoundParseOrc(csound, orc);
    ASSERT_TRUE(tree != NULL);

    TREE *instr = tree->next;
    ASSERT_TRUE(instr != NULL);
    ASSERT_TRUE(instr->right != NULL);

    // First statement: Sval = {{ ... }}
    TREE *stmt1 = instr->right;
    ASSERT_TRUE(stmt1->left != NULL && stmt1->left->value != NULL);
    ASSERT_STREQ("Sval", stmt1->left->value->lexeme);
    ASSERT_EQ(1u, stmt1->left->value->first_column);
    ASSERT_EQ(4u, stmt1->left->value->last_column);

    // The STRING_TOKEN (right child) should have columns of the
    // closing }} which is at columns 1-2 on its line.
    // BUG: without yycolumn reset in <xstr> newline, this will be 17-18
    ASSERT_TRUE(stmt1->right != NULL && stmt1->right->value != NULL);
    ASSERT_EQ(1u, stmt1->right->value->first_column);
    ASSERT_EQ(2u, stmt1->right->value->last_column);

    // Second statement: k1 = 440 (should be correct regardless,
    // since the newline after }} hits the INITIAL rule)
    TREE *stmt2 = stmt1->next;
    ASSERT_TRUE(stmt2 != NULL);
    ASSERT_TRUE(stmt2->left != NULL && stmt2->left->value != NULL);
    ASSERT_STREQ("k1", stmt2->left->value->lexeme);
    ASSERT_EQ(1u, stmt2->left->value->first_column);
    ASSERT_EQ(2u, stmt2->left->value->last_column);

    csoundDeleteTree(csound, tree);
}

// Test column tracking through rstr (R{ }R) multi-line strings.
// The <rstr> newline rule was already fixed with yycolumn = 1.
TEST_F (OrcCompileTests, testColumnNumbersRstr)
{
    //                                        R{ starts rstr
    const char *orc = "instr 1\n"                 // line 1
                      "Sval = R{\n"                // line 2: Sval col 1-4
                      "hello\n"                    // line 3: inside string
                      "}R\n"                       // line 4: }R at cols 1-2
                      "k1 = 440\n"                 // line 5
                      "endin\n";

    TREE *tree = csoundParseOrc(csound, orc);
    ASSERT_TRUE(tree != NULL);

    TREE *instr = tree->next;
    ASSERT_TRUE(instr != NULL);
    ASSERT_TRUE(instr->right != NULL);

    // First statement: Sval = R{ ... }R
    TREE *stmt1 = instr->right;
    ASSERT_TRUE(stmt1->left != NULL && stmt1->left->value != NULL);
    ASSERT_STREQ("Sval", stmt1->left->value->lexeme);

    // The STRING_TOKEN for rstr: }R at cols 1-2 on its line
    ASSERT_TRUE(stmt1->right != NULL && stmt1->right->value != NULL);
    ASSERT_EQ(1u, stmt1->right->value->first_column);
    ASSERT_EQ(2u, stmt1->right->value->last_column);

    // k1 after the rstr
    TREE *stmt2 = stmt1->next;
    ASSERT_TRUE(stmt2 != NULL);
    ASSERT_TRUE(stmt2->left != NULL && stmt2->left->value != NULL);
    ASSERT_STREQ("k1", stmt2->left->value->lexeme);
    ASSERT_EQ(1u, stmt2->left->value->first_column);
    ASSERT_EQ(2u, stmt2->left->value->last_column);

    csoundDeleteTree(csound, tree);
}

// Test that error messages contain column information
TEST_F (OrcCompileTests, testColumnNumbersInErrors)
{
    csoundCreateMessageBuffer(csound, 0);

    // Deliberate error: unknown opcode at a known column position
    //          col: 1234567890123456
    const char *orc = "instr 1\n"
                      "k1 badopcode 440\n"  // "badopcode" at cols 4-12
                      "endin\n";

    csoundCompileOrc(csound, orc);

    // Scan message buffer for column info
    bool found_column = false;
    while (csoundGetMessageCnt(csound)) {
        const char *msg = csoundGetFirstMessage(csound);
        if (msg && strstr(msg, "columns")) {
            found_column = true;
            printf("Error message with columns: %s\n", msg);
        }
        csoundPopFirstMessage(csound);
    }
    ASSERT_TRUE(found_column);
}

// Test that "unable to find opcode" error includes correct column range
TEST_F (OrcCompileTests, testErrorMsgUnknownOpcodeColumns)
{
    csoundCreateMessageBuffer(csound, 0);

    //          col: 1234567890123456
    const char *orc = "instr 1\n"
                      "k1 badopcode 440\n"  // "badopcode" at cols 4-12
                      "endin\n";

    csoundCompileOrc(csound, orc);

    // Collect all messages into one string for easier matching
    std::string allMessages;
    while (csoundGetMessageCnt(csound)) {
        const char *msg = csoundGetFirstMessage(csound);
        if (msg) allMessages += msg;
        csoundPopFirstMessage(csound);
    }

    // verify_opcode emits "columns %d-%d" with the opcode token columns
    // "badopcode" starts at col 4, ends at col 12
    ASSERT_TRUE(allMessages.find("columns 4-12") != std::string::npos)
        << "Expected 'columns 4-12' in error output, got:\n" << allMessages;
}

// Test that syntax errors (parser-level) include column info
TEST_F (OrcCompileTests, testErrorMsgSyntaxErrorColumns)
{
    csoundCreateMessageBuffer(csound, 0);

    //          col: 123456789
    const char *orc = "instr 1\n"
                      "k1 = +\n"  // syntax error: unexpected +
                      "endin\n";

    csoundCompileOrc(csound, orc);

    // Collect all messages
    std::string allMessages;
    while (csoundGetMessageCnt(csound)) {
        const char *msg = csoundGetFirstMessage(csound);
        if (msg) allMessages += msg;
        csoundPopFirstMessage(csound);
    }

    // csound_orcerror emits "columns %d,%d" (note: comma separator)
    ASSERT_TRUE(allMessages.find("columns") != std::string::npos)
        << "Expected 'columns' in syntax error output, got:\n" << allMessages;
}

// Test that error for wrong argument types includes column info
TEST_F (OrcCompileTests, testErrorMsgWrongArgsColumns)
{
    csoundCreateMessageBuffer(csound, 0);

    //          col: 123456789012345678
    const char *orc = "instr 1\n"
                      "k1 oscil \"bad\"\n"  // wrong arg type for oscil
                      "endin\n";

    csoundCompileOrc(csound, orc);

    // Collect all messages
    std::string allMessages;
    while (csoundGetMessageCnt(csound)) {
        const char *msg = csoundGetFirstMessage(csound);
        if (msg) allMessages += msg;
        csoundPopFirstMessage(csound);
    }

    // Should contain column info from verify_opcode error path
    ASSERT_TRUE(allMessages.find("columns") != std::string::npos)
        << "Expected 'columns' in arg-mismatch error output, got:\n" << allMessages;
    // "oscil" at cols 4-8
    ASSERT_TRUE(allMessages.find("columns 4-8") != std::string::npos)
        << "Expected 'columns 4-8' for 'oscil' in error output, got:\n" << allMessages;
}

// Test that error columns work correctly with leading whitespace
TEST_F (OrcCompileTests, testErrorMsgColumnsWithIndentation)
{
    csoundCreateMessageBuffer(csound, 0);

    //          col: 1234567890123456789012
    const char *orc = "instr 1\n"
                      "    k1 badopcode 440\n"  // "badopcode" at cols 8-16
                      "endin\n";

    csoundCompileOrc(csound, orc);

    // Collect all messages
    std::string allMessages;
    while (csoundGetMessageCnt(csound)) {
        const char *msg = csoundGetFirstMessage(csound);
        if (msg) allMessages += msg;
        csoundPopFirstMessage(csound);
    }

    // "badopcode" at cols 8-16 (4 spaces + "k1 " = 7, then badopcode at 8)
    ASSERT_TRUE(allMessages.find("columns 8-16") != std::string::npos)
        << "Expected 'columns 8-16' for indented 'badopcode', got:\n" << allMessages;
}

// Test that errors at different column offsets report the correct positions
TEST_F (OrcCompileTests, testErrorMsgDifferentColumnOffsets)
{
    csoundCreateMessageBuffer(csound, 0);

    //          col: 123456789012345678
    const char *orc = "instr 1\n"
                      "k1 bad1 440\n"       // "bad1" at cols 4-7
                      "endin\n";

    csoundCompileOrc(csound, orc);

    std::string allMessages;
    while (csoundGetMessageCnt(csound)) {
        const char *msg = csoundGetFirstMessage(csound);
        if (msg) allMessages += msg;
        csoundPopFirstMessage(csound);
    }

    // "bad1" at cols 4-7
    ASSERT_TRUE(allMessages.find("columns 4-7") != std::string::npos)
        << "Expected 'columns 4-7' for 'bad1', got:\n" << allMessages;

    // Now try a different column offset in a fresh instance
    csoundReset(csound);
    csoundCreateMessageBuffer(csound, 0);

    //          col: 123456789012345678901
    const char *orc2 = "instr 1\n"
                       "k1 = 1\n"
                       "   k2 badop2 440\n"  // "badop2" at cols 7-12
                       "endin\n";

    csoundCompileOrc(csound, orc2);

    std::string allMessages2;
    while (csoundGetMessageCnt(csound)) {
        const char *msg = csoundGetFirstMessage(csound);
        if (msg) allMessages2 += msg;
        csoundPopFirstMessage(csound);
    }

    // "badop2" at cols 7-12
    ASSERT_TRUE(allMessages2.find("columns 7-12") != std::string::npos)
        << "Expected 'columns 7-12' for 'badop2', got:\n" << allMessages2;
}

// Test column tracking across line continuation (\)
// Preprocessor converts \<LF> to inline #sline N directive;
// the <sline> rule must reset yycolumn so subsequent tokens
// get correct source columns.
TEST_F (OrcCompileTests, testColumnNumbersContinuation)
{
    //                   col: 12345678
    const char *orc = "instr 1\n"
                      "k1 = \\\n"           // line 2: k1 cols 1-2, \ joins next line
                      "     440\n"           // line 3: 440 at cols 6-8
                      "endin\n";

    TREE *tree = csoundParseOrc(csound, orc);
    ASSERT_TRUE(tree != NULL);

    TREE *instr = tree->next;
    ASSERT_TRUE(instr != NULL);
    ASSERT_TRUE(instr->right != NULL);

    TREE *stmt1 = instr->right;
    ASSERT_TRUE(stmt1->left != NULL && stmt1->left->value != NULL);
    ASSERT_STREQ("k1", stmt1->left->value->lexeme);
    ASSERT_EQ(1u, stmt1->left->value->first_column);
    ASSERT_EQ(2u, stmt1->left->value->last_column);

    // After \, the preprocessor emits #sline which resets yycolumn.
    // Five spaces of source indentation then advance to col 6.
    // "440" should be at columns 6-8.
    ASSERT_TRUE(stmt1->right != NULL && stmt1->right->value != NULL);
    ASSERT_STREQ("440", stmt1->right->value->lexeme);
    ASSERT_EQ(6u, stmt1->right->value->first_column);
    ASSERT_EQ(8u, stmt1->right->value->last_column);

    csoundDeleteTree(csound, tree);
}

// Test column tracking inside parentheses (ignorenewline state)
// Newlines inside parens should reset yycolumn but not return NEWLINE.
// Note: the parser expands (100 + 200) into an implicit ##add node
// with a temp #i0 variable, so the AST has 3 statements:
//   ##add(100, 200) -> #i0, k1 = #i0, k2 = 300
TEST_F (OrcCompileTests, testColumnNumbersIgnoreNewline)
{
    const char *orc = "instr 1\n"
                      "k1 = (100 +\n"      // line 2: ( triggers ignorenewline
                      " 200)\n"             // line 3: 200 at cols 2-4
                      "k2 = 300\n"          // line 4: k2 at cols 1-2
                      "endin\n";

    TREE *tree = csoundParseOrc(csound, orc);
    ASSERT_TRUE(tree != NULL);

    TREE *instr = tree->next;
    ASSERT_TRUE(instr != NULL);
    ASSERT_TRUE(instr->right != NULL);

    // First node is the implicit ##add operation.
    // Its right children contain the operands:
    //   100 at cols 7-9 (line 2), 200 at cols 2-4 (line 3, after newline)
    TREE *add_node = instr->right;
    ASSERT_TRUE(add_node->right != NULL && add_node->right->value != NULL);
    ASSERT_STREQ("100", add_node->right->value->lexeme);
    ASSERT_EQ(7u, add_node->right->value->first_column);
    ASSERT_EQ(9u, add_node->right->value->last_column);

    // "200" is the next operand of ##add
    ASSERT_TRUE(add_node->right->next != NULL && add_node->right->next->value != NULL);
    ASSERT_STREQ("200", add_node->right->next->value->lexeme);
    ASSERT_EQ(2u, add_node->right->next->value->first_column);
    ASSERT_EQ(4u, add_node->right->next->value->last_column);

    // Third statement: k2 = 300
    TREE *k2_stmt = add_node->next->next;
    ASSERT_TRUE(k2_stmt != NULL);
    ASSERT_TRUE(k2_stmt->left != NULL && k2_stmt->left->value != NULL);
    ASSERT_STREQ("k2", k2_stmt->left->value->lexeme);
    ASSERT_EQ(1u, k2_stmt->left->value->first_column);
    ASSERT_EQ(2u, k2_stmt->left->value->last_column);

    csoundDeleteTree(csound, tree);
}

// Test column tracking for regular quoted strings ("...")
// Note: flex input() used for string scanning bypasses YY_USER_ACTION,
// so the token columns reflect the opening quote position only.
// Next-line tokens are still correct because newline resets yycolumn.
TEST_F (OrcCompileTests, testColumnNumbersQuotedString)
{
    const char *orc = "instr 1\n"
                      "S1 = \"hello\"\n"     // line 2: S1 cols 1-2, "hello" starts at col 6
                      "k1 = 440\n"            // line 3: k1 at cols 1-2 (after newline reset)
                      "endin\n";

    TREE *tree = csoundParseOrc(csound, orc);
    ASSERT_TRUE(tree != NULL);

    TREE *instr = tree->next;
    ASSERT_TRUE(instr != NULL);
    ASSERT_TRUE(instr->right != NULL);

    // S1 = "hello"
    TREE *stmt1 = instr->right;
    ASSERT_TRUE(stmt1->left != NULL && stmt1->left->value != NULL);
    ASSERT_STREQ("S1", stmt1->left->value->lexeme);
    ASSERT_EQ(1u, stmt1->left->value->first_column);
    ASSERT_EQ(2u, stmt1->left->value->last_column);

    // The STRING_TOKEN first_column points to the opening quote
    ASSERT_TRUE(stmt1->right != NULL && stmt1->right->value != NULL);
    ASSERT_EQ(6u, stmt1->right->value->first_column);

    // k1 on the next line should have correct columns (newline resets yycolumn)
    TREE *stmt2 = stmt1->next;
    ASSERT_TRUE(stmt2 != NULL);
    ASSERT_TRUE(stmt2->left != NULL && stmt2->left->value != NULL);
    ASSERT_STREQ("k1", stmt2->left->value->lexeme);
    ASSERT_EQ(1u, stmt2->left->value->first_column);
    ASSERT_EQ(2u, stmt2->left->value->last_column);

    csoundDeleteTree(csound, tree);
}

TEST_F (OrcCompileTests, testStringsInEvent)
{
    const char* instrument = R"(
instr One
 S1 = p4
 S2 = p5
 i1 strcmp S1, "Three"
 i2 strcmp S2, "Two"
 if i1 != 0 && i2 != 0 then
  exitnow(-1)
 endif
endin
     )";

const char* event = R"(
    i "One" 0 1 "Three" "Two"
   )";

   int32_t result = csoundCompileOrc(csound, instrument);
   ASSERT_TRUE(result == 0);
   result = csoundStart(csound);
   ASSERT_TRUE(result == 0);
   csoundEventString(csound, event, 0);
   result = csoundPerformKsmps(csound);
   ASSERT_TRUE(result == 0);
}

TEST_F (OrcCompileTests, testMaxTableSize)
{
    const char* instrument = R"(
instr 1
a1 oscil 0,0,1
endin
     )";

const char* event = R"(
 f 1 0 [2^30-1] 2 0 [2^30-1] 0
 i1 0 1
   )";

   csoundEventString(csound, event, 0);
   int32_t result =
     csoundCompileOrc(csound, instrument);
     ASSERT_TRUE(result == 0);
     result = csoundStart(csound);
     ASSERT_TRUE(result == 0);
     result = csoundPerformKsmps(csound);
    if(sizeof(MYFLT) > 4) {
    ASSERT_TRUE(result == 0);
    ASSERT_TRUE(csoundTableLength(csound,1) == pow(2,30)-1);
   }
   else
    ASSERT_FALSE(result == 0);
}

TEST_F (OrcCompileTests, test0dbfs)
{
  const char* instrument = R"(
   0dbfs = 1
     )";

  int result = csoundCompileOrc(csound, instrument);
  ASSERT_TRUE(result == 0);
  MYFLT val = csoundGet0dBFS(csound);
  ASSERT_TRUE(val == 1.0);
}

TEST_F (OrcCompileTests, testReCompileCSD)
{
  const char* instrument = R"(
<CsoundSynthesizer>
<CsInstruments>

instr 1
endin

</CsInstruments>
<CsScore>
i 1 0 1000
</CsScore>
</CsoundSynthesizer>
     )";

  int32_t result = csoundCompileCSD(csound,instrument,1,0);
  ASSERT_TRUE(result == 0);
  result = csoundStart(csound);
  ASSERT_TRUE(result == 0);
  result = csoundPerformKsmps(csound);
  result = csoundCompileCSD(csound,instrument,1,0);
  ASSERT_TRUE(result == 0);
  result = csoundPerformKsmps(csound);
  ASSERT_TRUE(result == 0);
 }

TEST_F (OrcCompileTests, testSampleAccurate)
{
  const char* instrument = R"(
instr 1
a1 oscili 1, 440, -1, 0.25
out a1
endin
schedule(1,6/sr,0.5)
     )";


  int32_t result = csoundSetOption(csound, "--sample-accurate");
  ASSERT_TRUE(result == 0);
  result = csoundCompileOrc(csound, instrument);
  ASSERT_TRUE(result == 0);
  result = csoundStart(csound);
  ASSERT_TRUE(result == 0);
  result = csoundPerformKsmps(csound);
  const MYFLT *spout = csoundGetSpout(csound);
  ASSERT_TRUE(spout[5] == 0.0);
  ASSERT_TRUE(spout[6] == 1.0);


}

TEST_F (OrcCompileTests, testCompileCSD)
{
  const char* instrument = R"(
<CsoundSynthesizer>
<CsInstruments>

instr 1
endin

</CsInstruments>
<CsScore>
i 1 0 -1
</CsScore>
</CsoundSynthesizer>
     )";

  int32_t result = csoundCompileCSD(csound,instrument,1,0);
  ASSERT_TRUE(result == 0);
  result = csoundStart(csound);
  ASSERT_TRUE(result == 0);
  result = csoundPerformKsmps(csound);
}

TEST_F (OrcCompileTests, testAssert)
{
    int32_t result;
    const char* instrument =
        "instr 1 \n"
        "assert(0) \n"
        "assert(0) \n"
        "assert(0) \n"
        "assert(0) \n"
        "assert(0) \n"
        "assert(0) \n"
        "assert(1) \n"
        "endin \n";

    result = csoundSetOption(csound, "--run-unit-tests");
    ASSERT_TRUE(result == 0);
    result = csoundCompileOrc(csound, instrument);
    ASSERT_TRUE(result == 0);
    csoundReadScore(csound,  "i 1 0 0\n");
    result = csoundStart(csound);
    ASSERT_TRUE(result == 0);
    // Perform one k-cycle to execute the instrument
    csoundPerformKsmps(csound);
    ASSERT_EQ (6, csoundErrCnt(csound));
}
