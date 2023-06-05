#include <regex>

#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/WithColor.h"

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;

static llvm::cl::OptionCategory MyToolCategory("concrete-class options");
static llvm::cl::opt<std::string> clsName("class-name", llvm::cl::Required,
                                          llvm::cl::desc("class to concrete"),
                                          llvm::cl::cat(MyToolCategory));
static llvm::cl::opt<bool> declareOnly("declare-only",
                                       llvm::cl::desc("declare only"),
                                       llvm::cl::cat(MyToolCategory));
static llvm::cl::opt<std::string>
    outputFilename("o", llvm::cl::init("-"), llvm::cl::desc("output filename"),
                   llvm::cl::value_desc("filename"),
                   llvm::cl::cat(MyToolCategory));

class FunctionPrinter : public MatchFinder::MatchCallback {
private:
  llvm::raw_ostream &OS;

public:
  FunctionPrinter(llvm::raw_ostream &OS) : OS(OS) {}
  void inspectVirtualFunction(const clang::ASTContext *context,
                              const clang::CXXRecordDecl *cls) {
    auto &&SM = context->getSourceManager();
    for (auto &&m : cls->methods()) {
      if (!m->isVirtual() || !m->isPure()) {
        continue;
      }
      auto beginLocation = m->getSourceRange().getBegin();
      auto endLocation = m->getSourceRange().getEnd();
      if (!beginLocation.isValid() || !endLocation.isValid()) {
        continue;
      }
      beginLocation = SM.getSpellingLoc(beginLocation);
      endLocation = SM.getSpellingLoc(endLocation);
      endLocation = clang::Lexer::getLocForEndOfToken(endLocation, 0, SM,
                                                      context->getLangOpts());
      unsigned length =
          SM.getFileOffset(endLocation) - SM.getFileOffset(beginLocation);
      std::string sourceText =
          SM.getBufferOrNone(SM.getFileID(beginLocation))
              ->getBuffer()
              .substr(SM.getFileOffset(beginLocation), length)
              .str();
      if (!declareOnly) {
        const char v[] = "virtual";
        auto pos = sourceText.find(v);
        sourceText.replace(pos, sizeof(v), "");
        std::stringstream sbuf;
        std::regex_replace(std::ostreambuf_iterator<char>(sbuf),
                           sourceText.begin(), sourceText.end(),
                           std::regex("=\\s*0"), "");
        sourceText = sbuf.str() + " override";
      }

      OS << sourceText << ";\n";
    }

    for (auto &&item : cls->bases()) {
      const clang::Type *typePtr = item.getType().getTypePtr();
      if (typePtr->isRecordType() || typePtr->isClassType()) {
        inspectVirtualFunction(context, typePtr->getAsCXXRecordDecl());
      }
    }
  }

  void run(const MatchFinder::MatchResult &Result) override {
    if (auto &&cls = Result.Nodes.getNodeAs<clang::CXXRecordDecl>("root")) {
      inspectVirtualFunction(Result.Context, cls);
    }
  }
};

int main(int argc, const char **argv) {
  llvm::sys::PrintStackTraceOnErrorSignal(argv[0]);
  auto ExpectedParser = CommonOptionsParser::create(argc, argv, MyToolCategory,
                                                    llvm::cl::OneOrMore);
  if (!ExpectedParser) {
    llvm::WithColor::error() << llvm::toString(ExpectedParser.takeError());
    return 1;
  }
  CommonOptionsParser &OptionsParser = ExpectedParser.get();
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());

  std::error_code EC;
  llvm::ToolOutputFile outFile(outputFilename, EC, llvm::sys::fs::OF_None);
  if (EC) {
    llvm::errs() << EC.message() << '\n';
    return EC.value();
  }
  FunctionPrinter Printer(outFile.os());
  MatchFinder Finder;
  Finder.addMatcher(
      cxxRecordDecl(hasName(clsName), unless(isImplicit())).bind("root"),
      &Printer);
  if (int code = Tool.run(newFrontendActionFactory(&Finder).get())) {
    return code;
  }
  outFile.keep();
  return 0;
}
