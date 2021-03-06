# -*- coding: utf-8 -*-
from __future__ import print_function

import os

from .util import DRIVER_DIR


PORTFOLIO_DIR = os.path.join(DRIVER_DIR, "portfolios")

ALIASES = {}


ALIASES["xpp_blind"] = [
        "--heuristic", "hb=blind",
        "--search", "goal_relation([astar(hb)], heu=[hb], all_soft_goals=true)"
        ]

ALIASES["xpp_ff"] = [
        "--heuristic", "hff=ff(cache_estimates=false)",
        "--search", "goal_relation([lazy_greedy([hff], preferred=[hff])], heu=[hff], all_soft_goals=true)"
        ]

ALIASES["xpp_blind_uc"] = [
        "--heuristic", "h=hc(cache_estimates=false, nogoods=true, prune_subsumed_preconditions=false)",
        "--search", "goal_relation([dfs(learn=ucrn(hc=h))], heu=[h], all_soft_goals=true)"
        ]

ALIASES["xpp_blind_traps"] = [
        "--heuristic", "h=traps(cache_estimates=false)",
        "--search", "goal_relation([dfs(learn=trapsrn(trap=h))], heu=[h], all_soft_goals=true)"
        ]

ALIASES["xpp_hff_traps"] = [
        "--heuristic", "h1=hc(cache_estimates=false, nogoods=true, prune_subsumed_preconditions=false)",
        "--heuristic", "h2=traps(cache_estimates=false, evals=[h1])",
        "--heuristic", "hff=ff(cache_estimates=false)",
        "--search", "goal_relation([dfs(eval=hff, learn=trapsrn(trap=h2))], heu=[h1, h2, hff], all_soft_goals=true)"
        ]

ALIASES["xpp_ff_uc"] = [
        "--heuristic", "h=hc(cache_estimates=false, nogoods=true, prune_subsumed_preconditions=false)",
        "--heuristic", "hff=ff(cache_estimates=false)",
        "--search", "goal_relation([dfs(eval=hff, learn=trapsrn(trap=h2), preferred=hff)], heu=[hff, h], all_soft_goals=true)"
        ]

ALIASES["xpp_ids_ff_hc"] = [
        "--heuristic", "h=hc(cache_estimates=false, nogoods=true, prune_subsumed_preconditions=false)",
        "--heuristic", "hff=ff(cache_estimates=false)",
        "--search", "goal_relation([bounded_cost_dfs(max_bound=infinity, bound=4, step=2, eval=hff, learn=hcrn(hc=h), preferred=hff)], heu=[hff, h], all_soft_goals=true)"
        ]



ALIASES["seq-sat-fd-autotune-1"] = [
    "--heuristic", "hff=ff(transform=adapt_costs(one))",
    "--heuristic", "hcea=cea()",
    "--heuristic", "hcg=cg(transform=adapt_costs(plusone))",
    "--heuristic", "hgc=goalcount()",
    "--heuristic", "hAdd=add()",
    "--search", """iterated([
lazy(alt([single(sum([g(),weight(hff,10)])),
          single(sum([g(),weight(hff,10)]),pref_only=true)],
         boost=2000),
     preferred=[hff],reopen_closed=false,cost_type=one),
lazy(alt([single(sum([g(),weight(hAdd,7)])),
          single(sum([g(),weight(hAdd,7)]),pref_only=true),
          single(sum([g(),weight(hcg,7)])),
          single(sum([g(),weight(hcg,7)]),pref_only=true),
          single(sum([g(),weight(hcea,7)])),
          single(sum([g(),weight(hcea,7)]),pref_only=true),
          single(sum([g(),weight(hgc,7)])),
          single(sum([g(),weight(hgc,7)]),pref_only=true)],
         boost=1000),
     preferred=[hcea,hgc],reopen_closed=false,cost_type=one),
lazy(alt([tiebreaking([sum([g(),weight(hAdd,3)]),hAdd]),
          tiebreaking([sum([g(),weight(hAdd,3)]),hAdd],pref_only=true),
          tiebreaking([sum([g(),weight(hcg,3)]),hcg]),
          tiebreaking([sum([g(),weight(hcg,3)]),hcg],pref_only=true),
          tiebreaking([sum([g(),weight(hcea,3)]),hcea]),
          tiebreaking([sum([g(),weight(hcea,3)]),hcea],pref_only=true),
          tiebreaking([sum([g(),weight(hgc,3)]),hgc]),
          tiebreaking([sum([g(),weight(hgc,3)]),hgc],pref_only=true)],
         boost=5000),
     preferred=[hcea,hgc],reopen_closed=false,cost_type=normal),
eager(alt([tiebreaking([sum([g(),weight(hAdd,10)]),hAdd]),
           tiebreaking([sum([g(),weight(hAdd,10)]),hAdd],pref_only=true),
           tiebreaking([sum([g(),weight(hcg,10)]),hcg]),
           tiebreaking([sum([g(),weight(hcg,10)]),hcg],pref_only=true),
           tiebreaking([sum([g(),weight(hcea,10)]),hcea]),
           tiebreaking([sum([g(),weight(hcea,10)]),hcea],pref_only=true),
           tiebreaking([sum([g(),weight(hgc,10)]),hgc]),
           tiebreaking([sum([g(),weight(hgc,10)]),hgc],pref_only=true)],
          boost=500),
      preferred=[hcea,hgc],reopen_closed=true,cost_type=normal)
],repeat_last=true,continue_on_fail=true)"""]

ALIASES["seq-sat-fd-autotune-2"] = [
    "--heuristic", "hcea=cea(transform=adapt_costs(plusone))",
    "--heuristic", "hcg=cg(transform=adapt_costs(one))",
    "--heuristic", "hgc=goalcount(transform=adapt_costs(plusone))",
    "--heuristic", "hff=ff()",
    "--search", """iterated([
ehc(hcea,preferred=[hcea],preferred_usage=0,cost_type=normal),
lazy(alt([single(sum([weight(g(),2),weight(hff,3)])),
          single(sum([weight(g(),2),weight(hff,3)]),pref_only=true),
          single(sum([weight(g(),2),weight(hcg,3)])),
          single(sum([weight(g(),2),weight(hcg,3)]),pref_only=true),
          single(sum([weight(g(),2),weight(hcea,3)])),
          single(sum([weight(g(),2),weight(hcea,3)]),pref_only=true),
          single(sum([weight(g(),2),weight(hgc,3)])),
          single(sum([weight(g(),2),weight(hgc,3)]),pref_only=true)],
         boost=200),
     preferred=[hcea,hgc],reopen_closed=false,cost_type=one),
lazy(alt([single(sum([g(),weight(hff,5)])),
          single(sum([g(),weight(hff,5)]),pref_only=true),
          single(sum([g(),weight(hcg,5)])),
          single(sum([g(),weight(hcg,5)]),pref_only=true),
          single(sum([g(),weight(hcea,5)])),
          single(sum([g(),weight(hcea,5)]),pref_only=true),
          single(sum([g(),weight(hgc,5)])),
          single(sum([g(),weight(hgc,5)]),pref_only=true)],
         boost=5000),
     preferred=[hcea,hgc],reopen_closed=true,cost_type=normal),
lazy(alt([single(sum([g(),weight(hff,2)])),
          single(sum([g(),weight(hff,2)]),pref_only=true),
          single(sum([g(),weight(hcg,2)])),
          single(sum([g(),weight(hcg,2)]),pref_only=true),
          single(sum([g(),weight(hcea,2)])),
          single(sum([g(),weight(hcea,2)]),pref_only=true),
          single(sum([g(),weight(hgc,2)])),
          single(sum([g(),weight(hgc,2)]),pref_only=true)],
         boost=1000),
     preferred=[hcea,hgc],reopen_closed=true,cost_type=one)
],repeat_last=true,continue_on_fail=true)"""]

ALIASES["seq-sat-lama-2011"] = [
    "--if-unit-cost",
    "--heuristic",
    "hlm=lama_synergy(lm_rhw(reasonable_orders=true))",
    "--heuristic", "hff=ff_synergy(hlm)",
    "--search", """iterated([
                     lazy_greedy([hff,hlm],preferred=[hff,hlm]),
                     lazy_wastar([hff,hlm],preferred=[hff,hlm],w=5),
                     lazy_wastar([hff,hlm],preferred=[hff,hlm],w=3),
                     lazy_wastar([hff,hlm],preferred=[hff,hlm],w=2),
                     lazy_wastar([hff,hlm],preferred=[hff,hlm],w=1)
                     ],repeat_last=true,continue_on_fail=true)""",
    "--if-non-unit-cost",
    "--heuristic",
    "hlm1=lama_synergy(lm_rhw(reasonable_orders=true,"
    "                           lm_cost_type=one),transform=adapt_costs(one))",
    "--heuristic", "hff1=ff_synergy(hlm1)",
    "--heuristic",
    "hlm2=lama_synergy(lm_rhw(reasonable_orders=true,"
    "                           lm_cost_type=plusone),transform=adapt_costs(plusone))",
    "--heuristic", "hff2=ff_synergy(hlm2)",
    "--search", """iterated([
                     lazy_greedy([hff1,hlm1],preferred=[hff1,hlm1],
                                 cost_type=one,reopen_closed=false),
                     lazy_greedy([hff2,hlm2],preferred=[hff2,hlm2],
                                 reopen_closed=false),
                     lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=5),
                     lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=3),
                     lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=2),
                     lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=1)
                     ],repeat_last=true,continue_on_fail=true)""",
    "--always"]
# Append --always to be on the safe side if we want to append
# additional options later.

ALIASES["lama-first"] = [
    "--heuristic",
    """hlm=lama_synergy(lm_rhw(reasonable_orders=true,lm_cost_type=one),
                               transform=adapt_costs(one))""",
    "--heuristic", "hff=ff_synergy(hlm)",
    "--search", """lazy_greedy([hff,hlm],preferred=[hff,hlm],
                               cost_type=one,reopen_closed=false)"""]

ALIASES["seq-opt-bjolp"] = [
    "--search",
    "astar(lmcount(lm_merged([lm_rhw(),lm_hm(m=1)]),admissible=true),"
    "      mpd=true)"]

ALIASES["seq-opt-lmcut"] = [
    "--search", "astar(lmcut())"]


PORTFOLIOS = {}
for portfolio in os.listdir(PORTFOLIO_DIR):
    name, ext = os.path.splitext(portfolio)
    assert ext == ".py", portfolio
    PORTFOLIOS[name.replace("_", "-")] = os.path.join(PORTFOLIO_DIR, portfolio)


def show_aliases():
    for alias in sorted(ALIASES.keys() + PORTFOLIOS.keys()):
        print(alias)


def set_options_for_alias(alias_name, args):
    """
    If alias_name is an alias for a configuration, set args.search_options
    to the corresponding command-line arguments. If it is an alias for a
    portfolio, set args.portfolio to the path to the portfolio file.
    Otherwise raise KeyError.
    """
    assert not args.search_options
    assert not args.portfolio

    if alias_name in ALIASES:
        args.search_options = [x.replace(" ", "").replace("\n", "")
                               for x in ALIASES[alias_name]]
    elif alias_name in PORTFOLIOS:
        args.portfolio = PORTFOLIOS[alias_name]
    else:
        raise KeyError(alias_name)
