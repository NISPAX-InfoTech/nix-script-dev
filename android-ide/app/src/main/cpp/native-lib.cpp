#include <jni.h>
#include <iostream>
#include <sstream>
#include <string>
#include <exception>
#define NIX_ANDROID 1
#include "../../../../../src/main.cpp"

extern "C" JNIEXPORT jstring JNICALL
Java_in_nispax_nix_MainActivity_runNixScript(JNIEnv* env, jobject, jstring input, jstring runtimeInput) {
    const char* sourceChars = input ? env->GetStringUTFChars(input, nullptr) : nullptr;
    const char* inputChars = runtimeInput ? env->GetStringUTFChars(runtimeInput, nullptr) : nullptr;
    const std::string source = sourceChars ? sourceChars : "";
    const std::string providedInput = inputChars ? inputChars : "";
    if (sourceChars) env->ReleaseStringUTFChars(input, sourceChars);
    if (inputChars) env->ReleaseStringUTFChars(runtimeInput, inputChars);

    std::ostringstream output;
    std::istringstream suppliedInput(providedInput);
    auto* oldOut = std::cout.rdbuf(output.rdbuf());
    auto* oldErr = std::cerr.rdbuf(output.rdbuf());
    auto* oldIn = std::cin.rdbuf(suppliedInput.rdbuf());
    std::string result;
    try {
        nix::Lexer lexer(source);
        nix::Parser parser(lexer.scan());
        nix::Interpreter interpreter;
        interpreter.run(parser.parseMain());
        result = output.str();
        if (result.empty()) result = "Program finished successfully.";
    } catch (const std::exception& error) {
        result = output.str();
        if (!result.empty()) result += "\n";
        result += std::string("NixScript error: ") + error.what();
    } catch (...) {
        result = output.str() + "\nNixScript error: unknown native exception";
    }
    std::cout.rdbuf(oldOut);
    std::cerr.rdbuf(oldErr);
    std::cin.rdbuf(oldIn);
    return env->NewStringUTF(result.c_str());
}
