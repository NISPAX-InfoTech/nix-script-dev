#include <jni.h>
#include <iostream>
#include <sstream>
#include <string>

#define NIX_ANDROID
#include "../../../../../src/main.cpp"

extern "C"
JNIEXPORT jstring JNICALL
Java_com_nixscript_ide_MainActivity_runNixScript(JNIEnv* env, jclass, jstring input) {
    const char* chars = env->GetStringUTFChars(input, nullptr);
    if (chars == nullptr) return env->NewStringUTF("Could not read source text.");

    std::string source(chars);
    env->ReleaseStringUTFChars(input, chars);

    std::ostringstream output;
    std::streambuf* previousOut = std::cout.rdbuf(output.rdbuf());
    std::streambuf* previousErr = std::cerr.rdbuf(output.rdbuf());

    try {
        nix::Lexer lexer(source);
        nix::Parser parser(lexer.scan());
        nix::Interpreter interpreter;
        interpreter.run(parser.parseMain());
    } catch (const std::exception& error) {
        output << "NixScript error: " << error.what() << '\n';
    }

    std::cout.rdbuf(previousOut);
    std::cerr.rdbuf(previousErr);
    return env->NewStringUTF(output.str().c_str());
}
