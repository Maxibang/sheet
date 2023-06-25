#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>
#include <memory>
#include <cmath>
#include <vector>


using Value = std::variant<std::string, double, FormulaError>;

/* *** *** *** *** *** EmptyImpl CLASS *** *** *** *** *** */

CellInterface::Value EmptyImpl::GetValue() const {
    return {0.0};
}

std::string EmptyImpl::GetText() const {
    return {""};
}

void EmptyImpl::Clear() {
    // pass
}

CellType EmptyImpl::GetCellType() const {
     return CellType::empty;
}

std::vector<Position> EmptyImpl::GetReferencedCells() const {
    return {};
} 

void EmptyImpl::InvalidateCache() {
    // pass
}

bool EmptyImpl::Cached() const {
    return true;
}


/* *** *** *** *** *** TextImpl CLASS *** *** *** *** *** */

Value TextImpl::GetValue() const {
    if (!expression_.empty()) {
        if (expression_.front() == ESCAPE_SIGN) {
            return {expression_.substr(1)};
        }
    }
    return {expression_};
}

std::string TextImpl::GetText() const {
    return expression_;
}

void TextImpl::Clear() {
    expression_.clear();
}
    
CellType TextImpl::GetCellType() const {
    return CellType::text;
}
    
std::vector<Position> TextImpl::GetReferencedCells() const {
    return {};
}

void TextImpl::InvalidateCache() {
    // pass
}

bool TextImpl::Cached() const {
    return true;
}


/* *** *** *** *** *** FormulaImpl CLASS *** *** *** *** *** */
   
void FormulaImpl::Set(std::string text) {
    formula_ptr_ = ParseFormula(text);
}

Value FormulaImpl::GetValue() const {
    if (!Cached()) {
        auto res = formula_ptr_->Evaluate(sheet_);
        if (std::holds_alternative<double>(res))
        {
            if (std::isfinite(std::get<double>(res))) {
                return std::get<double>(res);
            }
            else {
                return FormulaError(FormulaError::Category::Div0);
            }
        }
        return std::get<FormulaError>(res);
    }
    return *cached_value_;
}

std::string FormulaImpl::GetText() const {
    return {FORMULA_SIGN + formula_ptr_->GetExpression()};
}

void FormulaImpl::Clear() {
    // pass
}
        
CellType FormulaImpl::GetCellType() const {
    return CellType::formula;
}
    
std::vector<Position> FormulaImpl::GetReferencedCells() const {
    return formula_ptr_.get()->GetReferencedCells();    
}

void FormulaImpl::InvalidateCache() {
    cached_value_.reset();
}

bool FormulaImpl::Cached() const {
    return cached_value_.has_value();
}


/* *** *** *** *** *** Cell CLASS *** *** *** *** *** */

void Cell::Set(std::string expr) {
    if (expr.empty()) {
        impl_ = std::make_unique<EmptyImpl>();
        return;
    }

    if (expr.at(0) != FORMULA_SIGN || (expr.at(0) == FORMULA_SIGN && expr.size() == 1)) {
        impl_ = std::make_unique<TextImpl>(expr);
        return;
    }

    try {
        impl_ = std::make_unique<FormulaImpl>(sheet_, std::string{ expr.begin() + 1, expr.end() });
    } catch (...) {
        throw FormulaException("Parsing error");
    }
}

void Cell::Clear() {
    impl_ = std::make_unique<EmptyImpl>();
}

Value Cell::GetValue() const {
    return impl_->GetValue();
}

std::string Cell::GetText() const {
    return impl_->GetText();
}
    
CellType Cell::GetCellType() const {
    return impl_->GetCellType();
}
    
bool Cell::HasCyclicDependency(const Cell* start_cell, const Position& end) const {
    for (const auto& referenced_cell : GetReferencedCells()) {
        if (referenced_cell == end) {
            return true;
        }

        auto ref_cell_ptr = dynamic_cast<const Cell*>(sheet_.GetCell(referenced_cell));

        if (!ref_cell_ptr) {
            sheet_.SetCell(referenced_cell, "");
            ref_cell_ptr = dynamic_cast<const Cell*>(sheet_.GetCell(referenced_cell));
        }
        
        if (ref_cell_ptr->HasCyclicDependency(start_cell, end)) {
            return true;
        }
    }
    return false;
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_.get()->GetReferencedCells();
}
    
void Cell::InvalidateCache() {
    impl_->InvalidateCache();
}

bool Cell::HasValidCache() const {
    return impl_->Cached();
}

Cell::~Cell() {
    impl_.release();
}