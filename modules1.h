#ifdef MOD_EXTRAS
int FuncTest();
int ModCloak(),UserModInfo();
int DispServInfo(), SysConfigHelp(), DispModInfo(), ModConfigHelp();
#endif
#ifdef MOD_MONITOR
int NewMonitor(), MonQuery(), MonSetPager();
int MonOnlineUsers(), MonHelp(), MonToggleCloak();
#endif
#ifdef MOD_VOTE
int Vote(), NoVote(), OpenVote(), CloseVote(), VoteResults();
int CloseMyPolls();
#endif
#ifdef MOD_STATS
int LoginStat(), PostStat(), PostwarStat(), LastLoginStat();
int ChatStat(), StatChat();
int UserStatDisplay(), SetUserStats();
#endif
#ifdef MOD_QUERY
int AdvQuery();
#endif
#ifdef MOD_POSTWAR
int PwPost(), PwPostMessage();
#endif
