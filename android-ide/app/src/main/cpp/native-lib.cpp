#include <jni.h>
#include <iostream>
#include <sstream>
#include <string>
#include <exception>
#define NIX_ANDROID 1
#include "../../../../../src/main.cpp"

extern "C" JNIEXPORT jstring JNICALL Java_in_nispax_nix_MainActivity_runNixScript(JNIEnv* env, jobject, jstring input) {
    const char* chars = env->GetStringUTFChars(input, nullptr);
    std::string source = chars ? chars : "";
    if (chars) env->ReleaseStringUTFChars(input, chars);
    std::ostringstream output;
    auto* oldOut = std::cout.rdbuf(output.rdbuf());
    std::string result;
    try {
        nix::Lexer lexer(source);
        nix::Parser parser(lexer.scan());
        nix::Interpreter interpreter;
        interpreter.run(parser.parseMain());
        result = output.str();
        if (result.empty()) result = "Program finished successfully.";
    } catch (const std::exception& error) { result = std::string("NixScript error: ") + error.what(); }
    std::cout.rdbuf(oldOut);
    return env->NewStringUTF(result.c_str());
}
