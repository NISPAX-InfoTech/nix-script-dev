#include <jni.h>
#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>

#define NIX_ANDROID 1
#include "../../../../../src/main.cpp"

extern "C" JNIEXPORT jstring JNICALL Java_in_nispax_nix_MainActivity_runNixScript(
    JNIEnv* env, jobject activity, jstring input) {
    const char* chars = env->GetStringUTFChars(input, nullptr);
    std::string source = chars ? chars : "";
    if (chars) env->ReleaseStringUTFChars(input, chars);

    jclass activityClass = env->GetObjectClass(activity);
    jmethodID requestInputMethod = env->GetMethodID(
        activityClass, "requestNixInput", "(Ljava/lang/String;)Ljava/lang/String;");
    if (requestInputMethod == nullptr && env->ExceptionCheck()) env->ExceptionClear();

    std::ostringstream output;
    auto* oldOut = std::cout.rdbuf(output.rdbuf());
    auto* oldErr = std::cerr.rdbuf(output.rdbuf());
    std::string result;

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
                std::string value = responseChars ? responseChars : "";
                if (responseChars) env->ReleaseStringUTFChars(response, responseChars);
                env->DeleteLocalRef(response);
                return value;
            });
        }
        interpreter.run(parser.parseMain());
        result = output.str();
        if (result.empty()) result = "Program finished successfully.";
    } catch (const std::exception& error) {
        result = std::string("NixScript error: ") + error.what();
        if (!output.str().empty()) result = output.str() + result;
    }

    std::cout.rdbuf(oldOut);
    std::cerr.rdbuf(oldErr);
    env->DeleteLocalRef(activityClass);
    return env->NewStringUTF(result.c_str());
}
