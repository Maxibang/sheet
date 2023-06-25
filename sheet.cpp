#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>
#include <string>

using namespace std::literals;
using namespace std;


// Cheking single cell existing
bool Sheet::CheckCellExist(const Position& pos) const {
    bool row_exists = pos.row < static_cast<int>(cells_.size());
    bool col_exists = false;
    
    if (row_exists) {
        col_exists = pos.col <  static_cast<int>(cells_.at(pos.row).size());
    }
    
    return row_exists && col_exists;
}


// Cheking out of range by access to definite cell 
void Sheet::CheckPosition(const Position& pos, string error_place) const {
    if (!pos.IsValid()) {
        error_place = " to INVALID Position: "s;
        error_place +=  "row: " + to_string(pos.row) + " column: " + to_string(pos.col);
        throw InvalidPositionException(error_place);
    }
}


// Expanding size of sheet if needed after inserting cell
void Sheet::ExpandPrintableSize(const Position& pos) {
    // Expand rows size
    int row_size = static_cast<int>(cells_.size());
    if (row_size < pos.row + 1) {
        cells_.reserve(pos.row + 1);
        cells_.resize(pos.row + 1);
        // OLD printable_size_.rows = pos.row + 1;
        printable_size_.rows = cells_.size();
    }
    
    // Expand columns size
    int col_size = static_cast<int>(cells_.at(pos.row).size());
    if (col_size < pos.col + 1) {
        for (auto& line : cells_) {
            if (static_cast<int>(line.size()) < pos.col + 1) {
                line.reserve(pos.col + 1);
                line.resize(pos.col + 1);
            }
        }
        
        if (printable_size_.cols < pos.col + 1) {
            printable_size_.cols = cells_.at(0).size();
        }
    }
}


// Delete empty rows
void Sheet::ResizeRows() {
    int row_size = cells_.size();
    for (int i = row_size - 1; i >= 0; --i) {
        if (all_of(begin(cells_.at(i)), end(cells_.at(i)), 
                   [](const auto& ptr) {
                       return ptr == nullptr;
                   })) {
            
            cells_.pop_back();
        } else {
            break;
        }
    }
    printable_size_.rows = static_cast<int>(cells_.size());
}


void Sheet::ResizeColumns() {
    int max = 0;
    for (int i = printable_size_.cols - 1; i >= 0; --i) {
        for (const auto& line :  cells_) {
            if (line.at(i) != nullptr) {
                max = i + 1;
                i = -1;
                break;
            }
        }
    }
    
    for_each(begin(cells_), end(cells_), [max](auto& line){
        line.resize(max);
    });
    
    printable_size_.cols = max;
}


Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    CheckPosition(pos, "Trying SetCell"s); 
    Cell* cell = (Cell*)(GetCell(pos));

    if (cell) {
        std::string prev_text = cell->GetText();

        InvalidateCellCache(pos);
        ClearDependencies(pos);

        cell->Clear();
        cell->Set(text);

        if (cell->HasCyclicDependency(cell, pos)) {
            cell->Set(std::move(prev_text));
            throw CircularDependencyException("New Cell has circular dependency");
        }

        for (const auto& ref_cell : cell->GetReferencedCells()) {
            AddDependency(ref_cell, pos);
        }
        
    } else {
        auto new_cell = std::make_unique<Cell>(*this);
        new_cell->Set(text);

        if (new_cell.get()->HasCyclicDependency(new_cell.get(), pos)) {
            throw CircularDependencyException("New Cell has circular dependency");
        }

        for (const auto& ref_cell : new_cell.get()->GetReferencedCells()) {
            AddDependency(ref_cell, pos);
        }
    
        ExpandPrintableSize(pos);
        cells_[pos.row][pos.col] = std::move(new_cell);
    }
    ResizeRows();
    ResizeColumns();
}


const CellInterface* Sheet::GetCell(Position pos) const {
    CheckPosition(pos, "Trying GetCell"s);
    if (CheckCellExist(pos)) {
        return cells_.at(pos.row).at(pos.col).get();;   
    }
    return nullptr;
}


CellInterface* Sheet::GetCell(Position pos) {
    CheckPosition(pos, "Trying GetCell"s);
    if (CheckCellExist(pos)) {
        return const_cast<Cell*>(cells_.at(pos.row).at(pos.col).get());   
    }
    return nullptr;
}


void Sheet::ClearCell(Position pos) {
    CheckPosition(pos, "Trying ClearCell"s);
    
    if (GetCell(pos) != nullptr) {
        cells_[pos.row][pos.col] = nullptr;
    } 
    ResizeRows();
    ResizeColumns();
    ClearDependencies(pos);
}


Size Sheet::GetPrintableSize() const {
    return printable_size_;
}


void Sheet::PrintValues(std::ostream& output) const {
    for (const auto& line : cells_) {
        bool is_first = true;
        for (const auto& cell : line) {
             if (!is_first) {
                output << '\t';
            }
            if (cell != nullptr) {
                switch(cell->GetCellType()) {
                    case CellType::text:
                        output << get<string>(cell->GetValue());
                        break;
                    case CellType::formula:
                        if (std::holds_alternative<double>(cell->GetValue())) {
                            output << get<double>(cell->GetValue());
                        } else {
                            output << get<FormulaError>(cell->GetValue()).ToString();
                        }
                        break;
                    case CellType::empty:
                        output << 0.0;
                        break;
                } 
            }
            is_first = false;
        }
        output << '\n';
    } 
}


void Sheet::PrintTexts(std::ostream& output) const {
    for (const auto& line : cells_) {
        bool is_first = true;
        for (const auto& cell : line) {
             if (!is_first) {
                output << '\t';
            }
            if (cell != nullptr) {
                output << cell->GetText();
            }
            is_first = false;
        }
        output << '\n';
    } 
}


std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}


void Sheet::InvalidateCellCache(const Position& pos) {
    for (const auto& dependent_cell : GetDependencies(pos)) {
        auto cell = (Cell*)GetCell(dependent_cell);
        cell->InvalidateCache();
        InvalidateCellCache(dependent_cell);
    }
}


void Sheet::AddDependency(const Position& cell, const Position& dependent_cell) {
    cells_dependencies_[cell].insert(dependent_cell);
}


const std::set<Position> Sheet::GetDependencies(const Position& pos) {
    if (cells_dependencies_.count(pos) != 0) {
        return cells_dependencies_.at(pos);
    }
    return {};
}


void Sheet::ClearDependencies(const Position& pos) {
    cells_dependencies_.erase(pos);
}
