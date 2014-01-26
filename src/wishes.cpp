//=======================================================================
// Copyright (c) 2013-2014 Baptiste Wicht.
// Distributed under the terms of the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include <iostream>
#include <fstream>
#include <sstream>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "wishes.hpp"
#include "objectives.hpp"
#include "expenses.hpp"
#include "earnings.hpp"
#include "fortune.hpp"
#include "accounts.hpp"
#include "args.hpp"
#include "data.hpp"
#include "guid.hpp"
#include "config.hpp"
#include "utils.hpp"
#include "console.hpp"
#include "budget_exception.hpp"

using namespace budget;

namespace {

static data_handler<wish> wishes;

void list_wishes(){
    if(wishes.data.size() == 0){
        std::cout << "No wishes" << std::endl;
    } else {
        std::vector<std::string> columns = {"ID", "Name", "Amount"};
        std::vector<std::vector<std::string>> contents;

        for(auto& wish : wishes.data){
            contents.push_back({to_string(wish.id), wish.name, to_string(wish.amount)});
        }

        display_table(columns, contents);
    }
}

void status_wishes(){
    std::cout << "Wishes" << std::endl << std::endl;

    auto today = boost::gregorian::day_clock::local_day();
    auto current_month = today.month();
    auto current_year = today.year();
    auto sm = start_month(current_year);

    size_t width = 0;
    for(auto& wish : wishes.data){
        width = std::max(rsize(wish.name), width);
    }

    budget::money fortune_amount;
    boost::gregorian::date fortune_date;
    for(auto& fortune : all_fortunes()){
        if(fortune_amount.zero()){
            fortune_amount = fortune.amount;
            fortune_date = fortune.check_date;
        } else {
            if(fortune.check_date > fortune_date){
                fortune_amount = fortune.amount;
                fortune_date = fortune.check_date;
            }
        }
    }

    budget::money year_expenses;
    for(auto& expense : all_expenses()){
        if(expense.date.year() == current_year && expense.date.month() >= sm && expense.date.month() <= current_month){
            year_expenses += expense.amount;
        }
    }

    budget::money year_earnings;
    for(auto& earning : all_earnings()){
        if(earning.date.year() == current_year && earning.date.month() >= sm && earning.date.month() <= current_month){
            year_earnings += earning.amount;
        }
    }

    auto year_balance = year_earnings - year_expenses;
    for(unsigned short i = sm; i <= current_month; ++i){
        boost::gregorian::greg_month month = i;

        auto current_accounts = all_accounts(current_year, month);
        for(auto& c : current_accounts){
            year_balance += c.amount;
        }
    }

    budget::money month_expenses;
    for(auto& expense : all_expenses()){
        if(expense.date.year() == current_year && expense.date.month() == current_month){
            month_expenses += expense.amount;
        }
    }

    budget::money month_earnings;
    for(auto& earning : all_earnings()){
        if(earning.date.year() == current_year && earning.date.month() == current_month){
            month_earnings += earning.amount;
        }
    }

    auto month_balance = month_earnings - month_expenses;
    for(auto& c : all_accounts(current_year, current_month)){
        month_balance += c.amount;
    }

    for(auto& wish : wishes.data){
        auto amount = wish.amount;

        std::cout << "  ";
        print_minimum(wish.name, width);
        std::cout << "  ";

        size_t monthly_breaks = 0;
        size_t yearly_breaks = 0;
        bool perfect_objective = true;

        for(auto& objective : all_objectives()){
            if(objective.type == "monthly"){
                auto success_before = budget::compute_success(month_balance, month_earnings, month_expenses, objective);
                auto success_after = budget::compute_success(month_balance - amount, month_earnings, month_expenses + amount, objective);

                if(success_before >= 100 && success_after < 100){
                    ++monthly_breaks;
                }

                if(success_after < 100){
                    perfect_objective = false;
                }
            } else if(objective.type == "yearly"){
                auto success_before = budget::compute_success(year_balance, year_earnings, year_expenses, objective);
                auto success_after = budget::compute_success(year_balance - amount, year_earnings, year_expenses + amount, objective);

                if(success_before >= 100 && success_after < 100){
                    ++yearly_breaks;
                }

                if(success_after < 100){
                    perfect_objective = false;
                }
            }
        }

        if(fortune_amount < wish.amount){
            std::cout << "Impossible (not enough fortune)";
        } else {
            if(month_balance > wish.amount){
                if(!all_objectives().empty()){
                    if(perfect_objective){
                        std::cout << "Perfect (On month balance, all objectives fullfilled)";
                    } else if(yearly_breaks > 0 || monthly_breaks > 0){
                        std::cout << "OK (On month balance, " << (yearly_breaks + monthly_breaks) << " objectives broken)";
                    } else if(yearly_breaks == 0 && monthly_breaks == 0){
                        std::cout << "Warning (On month balance, objectives not fullfilled)";
                    }
                } else {
                    std::cout << "OK (on month balance)";
                }
            } else if(year_balance > wish.amount){
                if(!all_objectives().empty()){
                    if(perfect_objective){
                        std::cout << "Perfect (On year balance, all objectives fullfilled)";
                    } else if(yearly_breaks > 0 || monthly_breaks > 0){
                        std::cout << "OK (On year balance, " << (yearly_breaks + monthly_breaks) << " objectives broken)";
                    } else if(yearly_breaks == 0 && monthly_breaks == 0){
                        std::cout << "Warning (On year balance, objectives not fullfilled)";
                    }
                } else {
                    std::cout << "OK (on year balance)";
                }
            } else {
                std::cout << "Warning (on fortune only)";
            }
        }

        std::cout << std::endl;
    }
}

void edit(budget::wish& wish){
    edit_string(wish.name, "Name");
    not_empty(wish.name, "The name of the wish cannot be empty");

    edit_money(wish.amount, "Amount");
}

} //end of anonymous namespace

void budget::wishes_module::load(){
    load_expenses();
    load_earnings();
    load_accounts();
    load_fortunes();
    load_objectives();
    load_wishes();
}

void budget::wishes_module::unload(){
    save_wishes();
}

void budget::wishes_module::handle(const std::vector<std::string>& args){
    if(args.size() == 1){
        status_wishes();
    } else {
        auto& subcommand = args[1];

        if(subcommand == "list"){
            list_wishes();
        } else if(subcommand == "status"){
            status_wishes();
        } else if(subcommand == "add"){
            wish wish;
            wish.guid = generate_guid();
            wish.date = boost::gregorian::day_clock::local_day();

            edit(wish);

            add_data(wishes, std::move(wish));
        } else if(subcommand == "delete"){
            enough_args(args, 3);

            std::size_t id = to_number<std::size_t>(args[2]);

            if(!exists(wishes, id)){
                throw budget_exception("There are no wish with id ");
            }

            remove(wishes, id);

            std::cout << "wish " << id << " has been deleted" << std::endl;
        } else if(subcommand == "edit"){
            enough_args(args, 3);

            std::size_t id = to_number<std::size_t>(args[2]);

            if(!exists(wishes, id)){
                throw budget_exception("There are no wish with id " + args[2]);
            }

            auto& wish = get(wishes, id);

            edit(wish);

            set_wishes_changed();
        } else {
            throw budget_exception("Invalid subcommand \"" + subcommand + "\"");
        }
    }
}

void budget::load_wishes(){
    load_data(wishes, "wishes.data");
}

void budget::save_wishes(){
    save_data(wishes, "wishes.data");
}

void budget::add_wish(budget::wish&& wish){
    add_data(wishes, std::forward<budget::wish>(wish));
}

std::ostream& budget::operator<<(std::ostream& stream, const wish& wish){
    return stream
        << wish.id  << ':'
        << wish.guid << ':'
        << wish.name << ':'
        << wish.amount << ':'
        << to_string(wish.date);
}

void budget::operator>>(const std::vector<std::string>& parts, wish& wish){
    wish.id = to_number<std::size_t>(parts[0]);
    wish.guid = parts[1];
    wish.name = parts[2];
    wish.amount = parse_money(parts[3]);
    wish.date = boost::gregorian::from_string(parts[4]);
}

std::vector<wish>& budget::all_wishes(){
    return wishes.data;
}

void budget::set_wishes_changed(){
    wishes.changed = true;
}
