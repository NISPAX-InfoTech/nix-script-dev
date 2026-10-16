#include <jni.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#define NIX_ANDROID
#include "../../../../../src/main.cpp"

extern "C"
JNIEXPORT jstring JNICALL
Java_com_nixscript_ide_MainActivity_runNixScript(JNIEnv* env, jobject activity, jstring input) {
    const char* chars = env->GetStringUTFChars(input, nullptr);
    if (chars == nullptr) return env->NewStringUTF("Could not read source text.");

    std::string source(chars);
    env->ReleaseStringUTFChars(input, chars);

    jclass activityClass = env->GetObjectClass(activity);
    jmethodID requestInputMethod = env->GetMethodID(
        activityClass, "requestNixInput", "(Ljava/lang/String;)Ljava/lang/String;");
    if (requestInputMethod == nullptr && env->ExceptionCheck()) env->ExceptionClear();

    std::ostringstream output;
    std::streambuf* previousOut = std::cout.rdbuf(output.rdbuf());
    std::streambuf* previousErr = std::cerr.rdbuf(output.rdbuf());

    try {
        nix::Lexer lexer(source);
        nix::Parser parser(lexer.scan());
        nix::Interpreter interpreter;
        if (requestInputMethod != nullptr) {
            interpreter.setInputProvider([&]() -> std::string {
                const std::string visibleOutput = output.str();
                jstring visibleOutputString = env->NewStringUTF(visibleOutput.c_str());
                if (visibleOutputString == nullptr) {
                    if (env->ExceptionCheck()) env->ExceptionClear();
                    throw std::runtime_error("Could not prepare the input prompt.");
                }

                jstring response = static_cast<jstring>(
                    env->CallObjectMethod(activity, requestInputMethod, visibleOutputString));
                env->DeleteLocalRef(visibleOutputString);
                if (env->ExceptionCheck()) {
                    env->ExceptionClear();
                    if (response != nullptr) env->DeleteLocalRef(response);
                    throw std::runtime_error("Android input dialog failed.");
                }
                if (response == nullptr) return {};

                const char* responseChars = env->GetStringUTFChars(response, nullptr);
                std::string value = responseChars != nullptr ? responseChars : "";
                if (responseChars != nullptr) env->ReleaseStringUTFChars(response, responseChars);
                env->DeleteLocalRef(response);
                return value;
            });
        }
        interpreter.run(parser.parseMain());
    } catch (const std::exception& error) {
        output << "NixScript error: " << error.what() << '\n';
    }

    std::cout.rdbuf(previousOut);
    std::cerr.rdbuf(previousErr);
    env->DeleteLocalRef(activityClass);
    return env->NewStringUTF(output.str().c_str());
}
