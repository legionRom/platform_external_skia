/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SKSL_VARIABLEREFERENCE
#define SKSL_VARIABLEREFERENCE

#include "SkSLBoolLiteral.h"
#include "SkSLConstructor.h"
#include "SkSLExpression.h"
#include "SkSLFloatLiteral.h"
#include "SkSLIRGenerator.h"
#include "SkSLIntLiteral.h"
#include "SkSLSetting.h"

namespace SkSL {

/**
 * A reference to a variable, through which it can be read or written. In the statement:
 *
 * x = x + 1;
 *
 * there is only one Variable 'x', but two VariableReferences to it.
 */
struct VariableReference : public Expression {
    enum RefKind {
        kRead_RefKind,
        kWrite_RefKind,
        kReadWrite_RefKind
    };

    VariableReference(int offset, const Variable& variable, RefKind refKind = kRead_RefKind)
    : INHERITED(offset, kVariableReference_Kind, variable.fType)
    , fVariable(variable)
    , fRefKind(refKind) {
        if (refKind != kRead_RefKind) {
            fVariable.fWriteCount++;
        }
        if (refKind != kWrite_RefKind) {
            fVariable.fReadCount++;
        }
    }

    ~VariableReference() override {
        if (fRefKind != kRead_RefKind) {
            fVariable.fWriteCount--;
        }
        if (fRefKind != kWrite_RefKind) {
            fVariable.fReadCount--;
        }
    }

    RefKind refKind() {
        return fRefKind;
    }

    void setRefKind(RefKind refKind) {
        if (fRefKind != kRead_RefKind) {
            fVariable.fWriteCount--;
        }
        if (fRefKind != kWrite_RefKind) {
            fVariable.fReadCount--;
        }
        if (refKind != kRead_RefKind) {
            fVariable.fWriteCount++;
        }
        if (refKind != kWrite_RefKind) {
            fVariable.fReadCount++;
        }
        fRefKind = refKind;
    }

    bool hasSideEffects() const override {
        return false;
    }

    bool isConstant() const override {
        return 0 != (fVariable.fModifiers.fFlags & Modifiers::kConst_Flag);
    }

    String description() const override {
        return fVariable.fName;
    }

    static std::unique_ptr<Expression> copy_constant(const IRGenerator& irGenerator,
                                                     const Expression* expr) {
        ASSERT(expr->isConstant());
        switch (expr->fKind) {
            case Expression::kIntLiteral_Kind:
                return std::unique_ptr<Expression>(new IntLiteral(irGenerator.fContext,
                                                                  -1,
                                                                  ((IntLiteral*) expr)->fValue));
            case Expression::kFloatLiteral_Kind:
                return std::unique_ptr<Expression>(new FloatLiteral(
                                                                   irGenerator.fContext,
                                                                   -1,
                                                                   ((FloatLiteral*) expr)->fValue));
            case Expression::kBoolLiteral_Kind:
                return std::unique_ptr<Expression>(new BoolLiteral(irGenerator.fContext,
                                                                   -1,
                                                                   ((BoolLiteral*) expr)->fValue));
            case Expression::kConstructor_Kind: {
                const Constructor* c = (const Constructor*) expr;
                std::vector<std::unique_ptr<Expression>> args;
                for (const auto& arg : c->fArguments) {
                    args.push_back(copy_constant(irGenerator, arg.get()));
                }
                return std::unique_ptr<Expression>(new Constructor(-1, c->fType,
                                                                   std::move(args)));
            }
            case Expression::kSetting_Kind: {
                const Setting* s = (const Setting*) expr;
                return std::unique_ptr<Expression>(new Setting(-1, s->fName,
                                                               copy_constant(irGenerator,
                                                                             s->fValue.get())));
            }
            default:
                ABORT("unsupported constant\n");
        }
    }

    std::unique_ptr<Expression> constantPropagate(const IRGenerator& irGenerator,
                                                  const DefinitionMap& definitions) override {
        if (fRefKind != kRead_RefKind) {
            return nullptr;
        }
        if ((fVariable.fModifiers.fFlags & Modifiers::kConst_Flag) && fVariable.fInitialValue &&
            fVariable.fInitialValue->isConstant()) {
            return copy_constant(irGenerator, fVariable.fInitialValue);
        }
        auto exprIter = definitions.find(&fVariable);
        if (exprIter != definitions.end() && exprIter->second &&
            (*exprIter->second)->isConstant()) {
            return copy_constant(irGenerator, exprIter->second->get());
        }
        return nullptr;
    }

    const Variable& fVariable;
    RefKind fRefKind;

private:
    typedef Expression INHERITED;
};

} // namespace

#endif
