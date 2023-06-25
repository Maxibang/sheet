#pragma once

#include "cell.h"
#include "common.h"

#include <map>
#include <set>
#include <vector>
#include <memory>
#include <functional>


class Sheet : public SheetInterface {
public:
    
    Sheet() = default;
    ~Sheet();  
    
    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

	// Можете дополнить ваш класс нужными полями и методами
    void CheckPosition(const Position& pos, std::string error_place) const;
    void ExpandPrintableSize(const Position& pos);
    bool CheckCellExist(const Position& pos) const;
    void ResizeRows();
    void ResizeColumns();    
    
    void InvalidateCellCache(const Position&);
    void AddDependency(const Position&, const Position&);
    const std::set<Position> GetDependencies(const Position&);
    void ClearDependencies(const Position&);
    
private:
	// Можете дополнить ваш класс нужными полями и методами
    std::vector<std::vector<std::unique_ptr<Cell>>> cells_;
    Size printable_size_{0, 0};
    
    std::map<Position, std::set<Position>> cells_dependencies_;
};