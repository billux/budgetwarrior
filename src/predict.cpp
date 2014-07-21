//=======================================================================
// Copyright (c) 2013-2014 Baptiste Wicht.
// Distributed under the terms of the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include <map>
#include <unordered_map>
#include <unordered_set>

#include "predict.hpp"
#include "overview.hpp"
#include "console.hpp"
#include "accounts.hpp"
#include "expenses.hpp"
#include "earnings.hpp"
#include "budget_exception.hpp"
#include "config.hpp"
#include "assert.hpp"

using namespace budget;

namespace {

void predict_overview(){
    auto today = budget::local_day();

    auto& expenses = all_expenses();
    auto& earnings = all_earnings();

    auto accounts = all_accounts(today.year(), today.month());

    std::map<std::size_t, std::size_t> account_mappings;

    std::vector<double> expense_multipliers(accounts.size(), 100.0);
    std::vector<double> earning_multipliers(accounts.size(), 100.0);

    std::cout << "Multipliers for expenses" << std::endl;

    std::size_t i = 0;
    for(auto& account : accounts){
        account_mappings[account.id] = i;

        std::cout << "   ";
        edit_double(expense_multipliers[i], account.name);

        ++i;
    }

    for(auto& expense : expenses){
        if(account_mappings.count(expense.account)){
            expense.amount *= (expense_multipliers[account_mappings[expense.account]] / 100.0);
        }
    }

    for(auto& earning : earnings){
        if(account_mappings.count(earning.account)){
            earning.amount *= (earning_multipliers[account_mappings[earning.account]] / 100.0);
        }
    }

    std::cout << std::endl;
    display_local_balance(today.year(), false);
    std::cout << std::endl;
    display_balance(today.year(), false);
    std::cout << std::endl;
    display_expenses(today.year(), false);
    std::cout << std::endl;
    display_earnings(today.year(), false);
    std::cout << std::endl;
}

} // end of anonymous namespace

void budget::predict_module::load(){
    load_accounts();
    load_expenses();
    load_earnings();
}

void budget::predict_module::handle(std::vector<std::string>& args){
    if(all_accounts().empty()){
        throw budget_exception("No accounts defined, you should start by defining some of them");
    }

    if(args.empty() || args.size() == 1){
        predict_overview();
    } else {
        throw budget_exception("Too many arguments");
    }
}
