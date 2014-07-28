/*
 * Copyright (C) 2010-2012 Wang Xiaohui <ewangplay@gmail.com>
 *                    
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#ifndef __FWITUX_H__
#define __FWITUX_H__

G_BEGIN_DECLS

#define G_STR_EMPTY(x) ((x) == NULL || (x)[0] == '\0')


/* Follow5 API Ver0.7 */
/* 申请的Follow5 api key */
#define FWITUX_API_KEY              "13AB014270B42AC0"

/* 获取最新分享 */
#define FWITUX_API_TIMELINE_PUBLIC	"http://api.follow5.com/api/statuses/public_timeline.xml"

/* 获取关注的人、好友和用户自己的分享 */
#define FWITUX_API_TIMELINE_FRIENDS	"http://api.follow5.com/api/statuses/friends_timeline.xml"

/* 获取用户自己的分享 */
#define FWITUX_API_MENTIONS_ME	    "http://api.follow5.com/api/statuses/mentions_me.xml"

/* 获取指定用户的分享，需要指定用户ID */
#define FWITUX_API_TIMELINE_USER	"http://api.follow5.com/api/statuses/user_timeline.xml"

/* 获取用户和好友的秘密分享 */
#define FWITUX_API_PRIVATE_FRIEND   "http://api.follow5.com/api/statuses/private_friend.xml"

/* 获取用户发送给自己的分享 */
#define FWITUX_API_PRIVATE_MINE     "http://api.follow5.com/api/statuses/private_mine.xml"

/* 获取一条指定的分享，需要指定分享的ID */
#define FWITUX_API_SPECIAL_STATUS   "http://api.follow5.com/api/statuses/show.xml"

/* 发布分享 */
#define FWITUX_API_POST_STATUS		"http://api.follow5.com/api/statuses/update.xml"

/* 删除分享，需要指定分享的ID */
#define FWITUX_API_DEL_STAUS        "http://api.follow5.com/api/statuses/destroy.xml"

/* 发布分享给指定好友 */
#define FWITUX_API_SEND_MESSAGE		"http://api.follow5.com/api/statuses/update_friend.xml"

/* 发布分享到组，组要指定组ID */
#define FWITUX_API_SEND_GROUP       "http://api.follow5.com/api/statuses/update_set.xml"

/* 显示用户的好友列表 */
#define FWITUX_API_FRIENDS		    "http://api.follow5.com/api/users/friends.xml"

/* 显示用户的被关注列表 */
#define FWITUX_API_FOLLOWED	    	"http://api.follow5.com/api/users/followed.xml"

/* 显示用户的关注列表 */
#define FWITUX_API_FOLLOWERS		"http://api.follow5.com/api/users/followers.xml"

/* 显示指定用户信息，需要指定用户ID */
#define FWITUX_API_SHOW_USER        "http://api.follow5.com/api/users/show.xml"

/* 添加好友 */
#define FWITUX_API_FRIEND_ADD       "http://api.follow5.com/api/friendships/create.xml"

/* 删除好友 */
#define FWITUX_API_FRIEND_DEL       "http://api.follow5.com/api/friendships/destroy.xml"

/* 添加关注用户 */
#define FWITUX_API_FOLLOWER_ADD	    "http://api.follow5.com/api/follow/create.xml"

/* 取消关注用户 */
#define FWITUX_API_FOLLOWER_DEL	    "http://api.follow5.com/api/follow/destroy.xml"

/* 判断是否为好友 */
#define FWITUX_API_IS_FRIEND        "http://api.follow5.com/api/friendships/exists.xml"

/* 判断是否关注 */
#define FWITUX_API_IS_FOLLOW        "http://api.follow5.com/api/follow/exists.xml"

/* 显示用户的组列表 */
#define FWITUX_API_LIST_GROUP       "http://api.follow5.com/api/set/list.xml"

/* 显示指定组的成员列表 */
#define FWITUX_API_LIST_GROUP_ACCOUNT "http://api.follow5.com/api/set/account_list.xml"

/* 用户帐号验证 */
#define FWITUX_API_LOGIN		    "http://api.follow5.com/api/users/verify_credentials.xml"

/* 获取指定分享的回复列表，需要指定分享ID */
#define FWITUX_API_TIMELINE_REPLY    "http://api.follow5.com/api/statuses/reply_timeline.xml"

/* 发布分享回复 */
#define FWITUX_API_REPLY_STATUS     "http://api.follow5.com/api/statuses/reply.xml"

/* 获取 回复我的 回复列表 */
#define FWITUX_API_TIMELINE_REPLIES "http://api.follow5.com/api/statuses/replies_timeline.xml"

/* 发送悄悄话，需要指定好友ID */
#define FWITUX_API_SEND_DIRECT_MESSAGE	"http://api.follow5.com/api/direct_messages/new.xml"

/* 删除悄悄话 */
#define FWITUX_API_DEL_DIRECT_MESSAGE "http://api.follow5.com/api/direct_messages/destroy.xml"

/* 显示悄悄话 */
#define FWITUX_API_SHOW_DIRECT_MESSAGES "http://api.follow5.com/api/direct_messages.xml"

/* Fwitux GConf Keys */
#define FWITUX_PREFS_PATH "/apps/fwitux"

#define FWITUX_PREFS_AUTH_USER                FWITUX_PREFS_PATH "/auth/user"
#define FWITUX_PREFS_AUTH_PASSWORD            FWITUX_PREFS_PATH "/auth/password"
#define FWITUX_PREFS_AUTH_AUTO_LOGIN          FWITUX_PREFS_PATH "/auth/auto_login"

#define FWITUX_PREFS_TWEETS_HOME_TIMELINE     FWITUX_PREFS_PATH "/tweets/home_timeline"
#define FWITUX_PREFS_TWEETS_RELOAD_TIMELINES  FWITUX_PREFS_PATH "/tweets/reload_timeline"
#define FWITUX_PREFS_TWEETS_SHOW_NAMES        FWITUX_PREFS_PATH "/tweets/names"

#define FWITUX_PREFS_UI_WINDOW_HEIGHT         FWITUX_PREFS_PATH "/ui/main_window_height"
#define FWITUX_PREFS_UI_WINDOW_WIDTH          FWITUX_PREFS_PATH "/ui/main_window_width"
#define FWITUX_PREFS_UI_WIN_POS_X             FWITUX_PREFS_PATH "/ui/main_window_pos_x"
#define FWITUX_PREFS_UI_WIN_POS_Y             FWITUX_PREFS_PATH "/ui/main_window_pos_y"
#define FWITUX_PREFS_UI_MAIN_WINDOW_HIDDEN	  FWITUX_PREFS_PATH "/ui/main_window_hidden"
#define FWITUX_PREFS_UI_EXPAND_MESSAGES       FWITUX_PREFS_PATH "/ui/expand_messages"
#define FWITUX_PREFS_UI_NOTIFICATION          FWITUX_PREFS_PATH "/ui/notify"
#define FWITUX_PREFS_UI_SOUND                 FWITUX_PREFS_PATH "/ui/sound"
#define FWITUX_PREFS_UI_SPELL_LANGUAGES       FWITUX_PREFS_PATH "/ui/spell_checker_languages"
#define FWITUX_PREFS_UI_SPELL                 FWITUX_PREFS_PATH "/ui/spell"

#define FWITUX_PREFS_HINTS_CLOSE_MAIN_WINDOW  FWITUX_PREFS_PATH "/hints/close_main_window"

/* Proxy configuration */
#define FWITUX_PROXY                          "/system/http_proxy"
#define FWITUX_PROXY_USE                      FWITUX_PROXY "/use_http_proxy"
#define FWITUX_PROXY_HOST                     FWITUX_PROXY "/host"
#define FWITUX_PROXY_PORT                     FWITUX_PROXY "/port"
#define FWITUX_PROXY_USE_AUTH                 FWITUX_PROXY "/use_authentication"
#define FWITUX_PROXY_USER                     FWITUX_PROXY "/authentication_user"
#define FWITUX_PROXY_PASS                     FWITUX_PROXY "/authentication_password"

/* File storage */
#define FWITUX_DIRECTORY                      "fwitux"
#define FWITUX_CACHE_IMAGES                   FWITUX_DIRECTORY "/avatars"

/* TweetListStore columns */
enum {
	PIXBUF_USER_PHOTO,
	STRING_FORMATED_TWEET,
	STRING_USER_NAME,
	STRING_STATUS_CREATED,
	STRING_STATUS_TEXT,
    STRING_STATUS_ID,
	STRING_USER_ID,
    STRING_STATUS_SOURCE,
    STRING_STATUS_LINK,
    STRING_STATUS_IMAGE_ADDRESS,
	N_COLUMNS
};

/* ReplyListStore columns */
enum {
	PIXBUF_REPLY_USER_PHOTO,
	STRING_REPLY_FORMATED_TWEET,
	STRING_REPLY_USER_NAME,
	STRING_REPLY_STATUS_CREATED,
	STRING_REPLY_STATUS_TEXT,
    STRING_REPLY_STATUS_ID,
	STRING_REPLY_USER_ID,
    STRING_REPLY_STATUS_SOURCE,
	N_REPLY_COLUMNS
};

/* Message dialog source type */
enum {
    MAIN_WINDOW_SOURCE,
    REPLY_WINDOW_SOURCE
};

G_END_DECLS

#endif /* __FWITUX_H__ */
