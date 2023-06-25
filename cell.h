#pragma once

#include "common.h"
#include "formula.h"

#include <variant>
#include <string>
#include <memory>
#include <vector>
#include <optional>


enum class CellType {
    text,
    formula, 
    empty
};


class Impl {
public:
    using Value = std::variant<std::string, double, FormulaError>;
    
    virtual ~Impl() = default;
    
    virtual Value GetValue() const = 0;
    virtual std::string GetText() const = 0;
    virtual void Clear() = 0;
    
    virtual CellType GetCellType() const = 0;
    
    virtual std::vector<Position> GetReferencedCells() const = 0;
    virtual void InvalidateCache() = 0;
    virtual bool Cached() const = 0;
    
};

class EmptyImpl : public Impl {
public:
    EmptyImpl() = default;
    
    CellInterface::Value GetValue() const override;
    std::string GetText() const override;
    void Clear() override;
    
    CellType GetCellType() const override;
    
    std::vector<Position> GetReferencedCells() const override; 
    void InvalidateCache() override;
    bool Cached() const override;
    
};


class TextImpl : public Impl {

public:
    TextImpl(std::string text) :  expression_(text) {}
    
    Value GetValue() const override;
    std::string GetText() const override;
    void Clear() override;
    
    CellType GetCellType() const override;
    
    std::vector<Position> GetReferencedCells() const override; 
    void InvalidateCache() override;
    bool Cached() const override;
    
private:
    std::string expression_ = ""; 
};


class FormulaImpl : public Impl {
public:
    explicit FormulaImpl(SheetInterface& sheet, std::string expression) : sheet_(sheet), formula_ptr_(ParseFormula(expression)) {}
    
    void Set(std::string text);
    Value GetValue() const override;
    std::string GetText() const override;
    void Clear() override;
        
    CellType GetCellType() const override;
    
    std::vector<Position> GetReferencedCells() const override; 
    void InvalidateCache() override;
    bool Cached() const override;
    
private:
    SheetInterface& sheet_;
    std::unique_ptr<FormulaInterface> formula_ptr_ = nullptr;
    std::optional<CellInterface::Value> cached_value_;
};


class Cell : public CellInterface {
public:
    Cell(SheetInterface& sheet) : CellInterface(), sheet_(sheet) {}
    ~Cell();

    void Set(std::string expr);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    
    CellType GetCellType() const;
    
    std::vector<Position> GetReferencedCells() const override;
    bool HasCyclicDependency(const Cell*, const Position&) const;
    
    void InvalidateCache();
    bool HasValidCache() const;
    

private:
    std::unique_ptr<Impl> impl_;
    SheetInterface& sheet_;
    
    
//можете воспользоваться нашей подсказкой, но это необязательно.
/*    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;
    std::unique_ptr<Impl> impl_;
*/
};