#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
#include <set>
#include "common.h"

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}


FormulaInterface::Value Formula::Evaluate(const SheetInterface& sheet) const {
    try {
        return ast_.Execute([&sheet](const Position& pos) {
            if (sheet.GetCell(pos) == nullptr) {
                        return 0.0;
            }
            auto value = sheet.GetCell(pos)->GetValue();
            if (std::holds_alternative<double>(value)) {
                return std::get<double>(value);
            } else if (std::holds_alternative<std::string>(value)) {
                try {
                    std::string tmp = std::get<std::string>(value);
                    if (std::all_of(tmp.cbegin(), tmp.cend(), [](char ch) { return (std::isdigit(ch) || ch == '.');})) {
                        return std::stod(tmp);
                    } else {
                        throw FormulaError(FormulaError::Category::Value);
                        return 0.0;
                    }
                } catch (...) {
                    throw FormulaError(FormulaError::Category::Value);
                }
            } else {
                throw std::get<FormulaError>(value);
            }
            return 0.0;
            });
        } catch (FormulaError& fe) {
            return fe;
        }
}

    
std::string Formula::GetExpression() const {
    std::stringstream strm;
    ast_.PrintFormula(strm);
    return strm.str();
} 


std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try {
        return std::make_unique<Formula>(std::move(expression));
    } catch (const std::exception&) {
        throw FormulaException("Formula parsing error!");
    }
}


std::vector<Position> Formula::GetReferencedCells() const {
    std::vector<Position> res;
    std::set<Position> cells(referenced_cells_.begin(), referenced_cells_.end());
    for (const auto& cell : cells) {
        res.push_back(cell);
    }
    return res;
}
