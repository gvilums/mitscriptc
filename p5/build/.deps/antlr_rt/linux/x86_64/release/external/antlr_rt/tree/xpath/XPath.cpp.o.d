{
    files = {
        "external/antlr_rt/tree/xpath/XPath.cpp"
    },
    depfiles_gcc = "build/.objs/antlr_rt/linux/x86_64/release/external/antlr_rt/tree/xpath/XPath.cpp.o:  external/antlr_rt/tree/xpath/XPath.cpp  external/antlr_rt/tree/xpath/XPathLexer.h  external/antlr_rt/antlr4-runtime.h external/antlr_rt/antlr4-common.h  external/antlr_rt/support/Guid.h  external/antlr_rt/support/Declarations.h  external/antlr_rt/ANTLRErrorListener.h  external/antlr_rt/RecognitionException.h external/antlr_rt/Exceptions.h  external/antlr_rt/ANTLRErrorStrategy.h external/antlr_rt/Token.h  external/antlr_rt/IntStream.h external/antlr_rt/ANTLRFileStream.h  external/antlr_rt/ANTLRInputStream.h external/antlr_rt/CharStream.h  external/antlr_rt/misc/Interval.h external/antlr_rt/BailErrorStrategy.h  external/antlr_rt/DefaultErrorStrategy.h  external/antlr_rt/misc/IntervalSet.h  external/antlr_rt/BaseErrorListener.h  external/antlr_rt/BufferedTokenStream.h external/antlr_rt/TokenStream.h  external/antlr_rt/CommonToken.h external/antlr_rt/WritableToken.h  external/antlr_rt/CommonTokenFactory.h external/antlr_rt/TokenFactory.h  external/antlr_rt/CommonTokenStream.h  external/antlr_rt/ConsoleErrorListener.h  external/antlr_rt/DiagnosticErrorListener.h  external/antlr_rt/FailedPredicateException.h  external/antlr_rt/InputMismatchException.h  external/antlr_rt/InterpreterRuleContext.h  external/antlr_rt/ParserRuleContext.h external/antlr_rt/RuleContext.h  external/antlr_rt/tree/ParseTree.h external/antlr_rt/support/Any.h  external/antlr_rt/support/CPPUtils.h external/antlr_rt/Lexer.h  external/antlr_rt/Recognizer.h external/antlr_rt/ProxyErrorListener.h  external/antlr_rt/support/Casts.h external/antlr_rt/TokenSource.h  external/antlr_rt/LexerInterpreter.h  external/antlr_rt/atn/PredictionContext.h external/antlr_rt/atn/ATN.h  external/antlr_rt/atn/ATNState.h external/antlr_rt/Vocabulary.h  external/antlr_rt/LexerNoViableAltException.h  external/antlr_rt/atn/ATNConfigSet.h external/antlr_rt/support/BitSet.h  external/antlr_rt/ListTokenSource.h  external/antlr_rt/NoViableAltException.h external/antlr_rt/Parser.h  external/antlr_rt/tree/ParseTreeListener.h  external/antlr_rt/ParserInterpreter.h  external/antlr_rt/RuleContextWithAltNum.h  external/antlr_rt/RuntimeMetaData.h  external/antlr_rt/TokenStreamRewriter.h  external/antlr_rt/UnbufferedCharStream.h  external/antlr_rt/UnbufferedTokenStream.h  external/antlr_rt/atn/ATNConfig.h  external/antlr_rt/atn/ATNDeserializationOptions.h  external/antlr_rt/atn/ATNDeserializer.h  external/antlr_rt/atn/LexerAction.h  external/antlr_rt/atn/LexerActionType.h  external/antlr_rt/atn/ATNSerializer.h  external/antlr_rt/atn/ATNSimulator.h external/antlr_rt/atn/ATNType.h  external/antlr_rt/atn/AbstractPredicateTransition.h  external/antlr_rt/atn/Transition.h  external/antlr_rt/atn/ActionTransition.h  external/antlr_rt/atn/AmbiguityInfo.h  external/antlr_rt/atn/DecisionEventInfo.h  external/antlr_rt/atn/ArrayPredictionContext.h  external/antlr_rt/atn/AtomTransition.h  external/antlr_rt/atn/BasicBlockStartState.h  external/antlr_rt/atn/BlockStartState.h  external/antlr_rt/atn/DecisionState.h external/antlr_rt/atn/BasicState.h  external/antlr_rt/atn/BlockEndState.h  external/antlr_rt/atn/ContextSensitivityInfo.h  external/antlr_rt/atn/DecisionInfo.h  external/antlr_rt/atn/PredicateEvalInfo.h  external/antlr_rt/atn/ErrorInfo.h  external/antlr_rt/atn/EmptyPredictionContext.h  external/antlr_rt/atn/SingletonPredictionContext.h  external/antlr_rt/atn/EpsilonTransition.h  external/antlr_rt/atn/LL1Analyzer.h  external/antlr_rt/atn/LexerATNConfig.h  external/antlr_rt/atn/LexerATNSimulator.h  external/antlr_rt/atn/LexerActionExecutor.h  external/antlr_rt/atn/LexerChannelAction.h  external/antlr_rt/atn/LexerCustomAction.h  external/antlr_rt/atn/LexerIndexedCustomAction.h  external/antlr_rt/atn/LexerModeAction.h  external/antlr_rt/atn/LexerMoreAction.h  external/antlr_rt/atn/LexerPopModeAction.h  external/antlr_rt/atn/LexerPushModeAction.h  external/antlr_rt/atn/LexerSkipAction.h  external/antlr_rt/atn/LexerTypeAction.h  external/antlr_rt/atn/LookaheadEventInfo.h  external/antlr_rt/atn/LoopEndState.h  external/antlr_rt/atn/NotSetTransition.h  external/antlr_rt/atn/SetTransition.h  external/antlr_rt/atn/OrderedATNConfigSet.h  external/antlr_rt/atn/ParseInfo.h  external/antlr_rt/atn/ParserATNSimulator.h  external/antlr_rt/atn/PredictionMode.h external/antlr_rt/dfa/DFAState.h  external/antlr_rt/atn/SemanticContext.h  external/antlr_rt/atn/PlusBlockStartState.h  external/antlr_rt/atn/PlusLoopbackState.h  external/antlr_rt/atn/PrecedencePredicateTransition.h  external/antlr_rt/atn/PredicateTransition.h  external/antlr_rt/atn/ProfilingATNSimulator.h  external/antlr_rt/atn/RangeTransition.h  external/antlr_rt/atn/RuleStartState.h  external/antlr_rt/atn/RuleStopState.h  external/antlr_rt/atn/RuleTransition.h  external/antlr_rt/atn/StarBlockStartState.h  external/antlr_rt/atn/StarLoopEntryState.h  external/antlr_rt/atn/StarLoopbackState.h  external/antlr_rt/atn/TokensStartState.h  external/antlr_rt/atn/WildcardTransition.h external/antlr_rt/dfa/DFA.h  external/antlr_rt/dfa/DFASerializer.h  external/antlr_rt/dfa/LexerDFASerializer.h  external/antlr_rt/misc/InterpreterDataReader.h  external/antlr_rt/misc/MurmurHash.h external/antlr_rt/misc/Predicate.h  external/antlr_rt/support/Arrays.h  external/antlr_rt/support/StringUtils.h  external/antlr_rt/tree/AbstractParseTreeVisitor.h  external/antlr_rt/tree/ParseTreeVisitor.h  external/antlr_rt/tree/ErrorNode.h external/antlr_rt/tree/TerminalNode.h  external/antlr_rt/tree/ErrorNodeImpl.h  external/antlr_rt/tree/TerminalNodeImpl.h  external/antlr_rt/tree/ParseTreeProperty.h  external/antlr_rt/tree/ParseTreeWalker.h external/antlr_rt/tree/Trees.h  external/antlr_rt/tree/pattern/Chunk.h  external/antlr_rt/tree/pattern/ParseTreeMatch.h  external/antlr_rt/tree/pattern/ParseTreePattern.h  external/antlr_rt/tree/pattern/ParseTreePatternMatcher.h  external/antlr_rt/tree/pattern/RuleTagToken.h  external/antlr_rt/tree/pattern/TagChunk.h  external/antlr_rt/tree/pattern/TextChunk.h  external/antlr_rt/tree/pattern/TokenTagToken.h  external/antlr_rt/tree/xpath/XPath.h  external/antlr_rt/tree/xpath/XPathElement.h  external/antlr_rt/tree/xpath/XPathLexerErrorListener.h  external/antlr_rt/tree/xpath/XPathRuleAnywhereElement.h  external/antlr_rt/tree/xpath/XPathRuleElement.h  external/antlr_rt/tree/xpath/XPathTokenAnywhereElement.h  external/antlr_rt/tree/xpath/XPathTokenElement.h  external/antlr_rt/tree/xpath/XPathWildcardAnywhereElement.h  external/antlr_rt/tree/xpath/XPathWildcardElement.h\
",
    values = {
        "/usr/bin/gcc",
        {
            "-m64",
            "-fvisibility=hidden",
            "-fvisibility-inlines-hidden",
            "-O3",
            "-std=c++2a",
            "-Iexternal/antlr_rt",
            "-DNDEBUG"
        }
    }
}