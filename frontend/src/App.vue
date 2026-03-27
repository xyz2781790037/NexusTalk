<template>
  <div class="h-screen w-full flex items-center justify-center bg-gray-100 relative">
    
    <div class="fixed top-5 left-1/2 transform -translate-x-1/2 z-[100] flex flex-col gap-2 pointer-events-none">
      <div v-for="t in toasts" :key="t.id" 
           :class="['px-6 py-3 rounded shadow-lg text-white text-sm font-bold transition-all duration-300', t.type === 'error' ? 'bg-red-500' : 'bg-green-500']">
        {{ t.msg }}
      </div>
    </div>

    <div v-if="showImagePreview" class="fixed inset-0 z-[200] bg-black/90 flex items-center justify-center p-4 cursor-zoom-out transition-opacity" @click="showImagePreview = false">
      <img :src="previewImageUrl" class="max-w-full max-h-full object-contain shadow-2xl" @click.stop />
      <button class="absolute top-6 right-6 text-white text-3xl font-bold hover:text-gray-300 transition" @click="showImagePreview = false">✕</button>
    </div>

    <div v-if="!isLoggedIn" class="bg-white p-8 rounded-lg shadow-lg w-96">
      <h2 class="text-2xl font-bold mb-6 text-center text-gray-800">
        {{ isRegisterMode ? '用户注册' : 'ZealChat 分布式聊天' }}
      </h2>
      <div class="mb-4">
        <label class="block text-sm font-medium text-gray-700">账号 (输入纯数字或字母)</label>
        <input v-model="loginForm.account" @keyup.enter="handleSubmit" type="text" class="mt-1 block w-full border border-gray-300 rounded-md p-2 focus:ring-blue-500 focus:border-blue-500">
      </div>
      <div :class="isRegisterMode ? 'mb-4' : 'mb-6'">
        <label class="block text-sm font-medium text-gray-700">密码</label>
        <input v-model="loginForm.password" @keyup.enter="handleSubmit" type="password" class="mt-1 block w-full border border-gray-300 rounded-md p-2 focus:ring-blue-500 focus:border-blue-500">
      </div>
      
      <div v-if="isRegisterMode" class="mb-4">
        <label class="block text-sm font-medium text-gray-700">邮箱</label>
        <input v-model="loginForm.email" @keyup.enter="handleSubmit" placeholder="例如: abc@qq.com" type="text" class="mt-1 block w-full border border-gray-300 rounded-md p-2 focus:ring-blue-500 focus:border-blue-500">
      </div>

      <div v-if="isRegisterMode" class="mb-6">
        <label class="block text-sm font-medium text-gray-700">验证码</label>
        <div class="flex gap-2 mt-1">
          <input v-model="loginForm.code" @keyup.enter="handleSubmit" type="text" placeholder="输入验证码" class="flex-1 border border-gray-300 rounded-md p-2 focus:ring-blue-500 focus:border-blue-500">
          <button @click="sendVerifyCode" :disabled="countdown > 0" class="px-4 py-2 bg-gray-100 text-gray-700 rounded-md hover:bg-gray-200 transition disabled:opacity-50 whitespace-nowrap font-medium text-sm border border-gray-200">
            {{ countdown > 0 ? `${countdown}s 后重发` : '获取验证码' }}
          </button>
        </div>
      </div>

      <button @click="handleSubmit" class="w-full bg-blue-600 text-white font-bold py-2 px-4 rounded hover:bg-blue-700 transition">
        {{ isRegisterMode ? '立即注册' : '登 录' }}
      </button>
      <p class="mt-4 text-center text-sm text-gray-600">
        {{ isRegisterMode ? '已有账号？' : '没有账号？' }}
        <span @click="isRegisterMode = !isRegisterMode" class="text-blue-500 cursor-pointer hover:underline">
          {{ isRegisterMode ? '去登录' : '立即注册' }}
        </span>
      </p>
    </div>

    <div v-else class="flex h-[90vh] w-[90vw] max-w-6xl bg-white rounded-xl shadow-2xl overflow-hidden border border-gray-200">
      
      <div class="w-20 bg-gray-900 flex flex-col items-center py-6 border-r border-gray-800">
        <div @click="showProfileModal = true" class="w-12 h-12 rounded-full flex items-center justify-center text-white font-bold text-xl mb-8 cursor-pointer hover:ring-2 ring-white transition shrink-0 shadow-lg shadow-black/20" :style="{ backgroundColor: myInfo.color || '#3b82f6' }">
          {{ myInfo.myname ? myInfo.myname.charAt(0) : 'U' }}
        </div>
        
        <div class="flex-1 space-y-6 flex flex-col">
          <button @click="switchTab('chat')" :class="['transition', currentTab === 'chat' ? 'text-white' : 'text-gray-500 hover:text-gray-300']">
            <MessageCircle class="w-8 h-8 mx-auto" />
          </button>
          <button @click="switchTab('friends')" :class="['transition', currentTab === 'friends' ? 'text-white' : 'text-gray-500 hover:text-gray-300']">
            <User class="w-8 h-8 mx-auto" />
          </button>
          <button @click="switchTab('groups')" :class="['transition', currentTab === 'groups' ? 'text-white' : 'text-gray-500 hover:text-gray-300']">
            <Users class="w-8 h-8 mx-auto" />
          </button>
          
          <button @click="openNotifModal" class="text-gray-500 hover:text-blue-400 transition relative mt-auto">
            <Bell class="w-8 h-8 mx-auto" />
            <span v-if="notifications.length > 0" class="absolute top-0 right-2 w-3 h-3 bg-red-500 rounded-full border-2 border-gray-900"></span>
          </button>
        </div>
        <button @click="showLogoutConfirmModal = true" class="text-gray-500 hover:text-red-400 transition mt-6"><LogOut class="w-8 h-8" /></button>
      </div>

      <div class="w-64 bg-gray-50 border-r border-gray-200 flex flex-col">
        <div class="h-16 flex items-center justify-between px-4 border-b border-gray-200">
          <span class="font-bold text-lg text-gray-800">
            {{ currentTab === 'chat' ? '最近消息' : (currentTab === 'friends' ? '我的好友' : '我的群聊') }}
          </span>
          <div class="flex gap-2">
            <button @click="switchTab(currentTab)" class="text-gray-400 hover:text-blue-500 transition text-xs border border-gray-200 px-2 rounded">刷新</button>
            <button @click="openAddModal" class="text-gray-500 hover:text-blue-500 transition"><PlusCircle class="w-6 h-6" /></button>
          </div>
        </div>
        
        <div class="flex-1 overflow-y-auto">
          <template v-if="currentTab === 'chat'">
            <div v-if="recentChats.length === 0" class="text-center text-gray-400 mt-10 text-sm">暂无新消息</div>
            <div v-for="chat in recentChats" :key="chat.id" @click="selectContactFromRecent(chat)"
                 :class="['flex items-center p-4 cursor-pointer transition border-l-4', currentContact?.id === chat.id ? 'border-blue-500 bg-gray-200' : 'border-transparent hover:bg-gray-100']">
              <div :class="['w-10 h-10 rounded-full mr-3 flex items-center justify-center text-white font-bold text-sm shrink-0', chat.type === 'group' ? 'bg-purple-500' : 'bg-green-500']" :style="{ backgroundColor: chat.color }">
                {{ chat.type === 'group' ? '群' : chat.avatar }}
              </div>
              <div class="overflow-hidden flex-1">
                <div class="flex justify-between items-center">
                  <div class="font-bold text-gray-800 text-sm truncate">{{ chat.title }}</div>
                  <div class="text-[10px] text-gray-400">{{ chat.time }}</div>
                </div>
                <div class="text-xs text-gray-500 mt-1 flex justify-between">
                  <span class="truncate mr-2">{{ chat.lastMsg }}</span>
                  <span v-if="chat.unread > 0" class="bg-red-500 text-white text-[10px] px-1.5 py-0.5 rounded-full shrink-0">{{ chat.unread }}</span>
                </div>
              </div>
            </div>
          </template>

          <template v-if="currentTab === 'friends'">
            <div v-if="friendsList.length === 0" class="text-center text-gray-400 mt-10 text-sm">暂无好友</div>
            <div v-for="friend in friendsList" :key="friend.name" @click="selectContact(friend, 'friend')"
                 :class="['flex items-center p-4 cursor-pointer transition border-l-4', currentContact?.rawName === friend.name ? 'border-blue-500 bg-gray-200' : 'border-transparent hover:bg-gray-100']">
              <div class="w-10 h-10 rounded-full mr-3 flex items-center justify-center text-white font-bold text-sm shrink-0" :style="{ backgroundColor: friend.color }">
                {{ friend.myname ? friend.myname.charAt(0) : 'F' }}
              </div>
              <div class="overflow-hidden">
                <div class="font-bold text-gray-800 text-sm truncate">{{ friend.myname }}</div>
                <div class="text-xs flex items-center gap-1 mt-1">
                  <span :class="['w-2 h-2 rounded-full', friend.state === 'online' ? 'bg-green-500' : 'bg-gray-400']"></span>
                  <span class="text-gray-500">{{ friend.state === 'online' ? '在线' : '离线' }}</span>
                </div>
              </div>
            </div>
          </template>

          <template v-if="currentTab === 'groups'">
            <div v-if="groupsList.length === 0" class="text-center text-gray-400 mt-10 text-sm">暂无群聊</div>
            <div v-for="group in groupsList" :key="group.name" @click="selectContact(group, 'group')"
                 :class="['flex items-center p-4 cursor-pointer transition border-l-4', currentContact?.rawName === group.name ? 'border-blue-500 bg-gray-200' : 'border-transparent hover:bg-gray-100']">
              <div class="w-10 h-10 rounded-full mr-3 flex items-center justify-center text-white font-bold text-sm shrink-0" :style="{ backgroundColor: group.color }">群</div>
              <div class="overflow-hidden">
                <div class="font-bold text-gray-800 text-sm truncate">{{ group.name.replace('grop:', '') }}</div>
                <div class="text-xs text-gray-500">{{ group.amount }} 条未读消息</div>
              </div>
            </div>
          </template>
        </div>
      </div>

      <div class="flex-1 flex flex-col bg-[#f5f5f5] h-full overflow-hidden">
        <template v-if="currentContact">
          <div class="h-16 border-b border-gray-200 flex items-center justify-between px-6 bg-gray-50 shrink-0">
            <div class="flex items-center gap-2 overflow-hidden">
              <span class="font-bold text-xl text-gray-800 truncate">
                {{ currentContactType === 'friend' ? currentContact.myname : currentContact.id.replace('grop:', '') }}
              </span>
              <span v-if="currentContactType === 'group'" class="text-sm font-normal text-gray-500 shrink-0 mt-1">
                ({{ currentGroupMembers.length > 0 ? currentGroupMembers.length : '加载中...' }} 人)
              </span>
            </div>
            
            <button v-if="currentContactType === 'group'" @click="openGroupSettings" class="text-gray-500 hover:text-blue-500 transition"><Settings class="w-6 h-6" /></button>
            <button v-if="currentContactType === 'friend' && currentContact.id !== 'user:' + loginForm.account" @click="showFriendSettingsModal = true" class="text-gray-500 hover:text-blue-500 transition">
              <Settings class="w-6 h-6" />
            </button>
          </div>

          <div class="flex-1 overflow-y-auto p-6 space-y-6 relative" ref="messageContainer" @scroll="handleChatScroll">
            <div v-if="isLoadingHistory" class="text-center text-xs text-blue-400 my-2 py-1 bg-blue-50 rounded-full w-32 mx-auto">
              <span class="inline-block animate-pulse">正在加载数据...</span>
            </div>
            <div v-if="!hasMoreHistory && messages.length > 0 && !isLoadingHistory" class="text-center text-xs text-gray-300 my-2">
              ------ 已经到底了 ------
            </div>

            <div v-for="(msg, index) in messages" :key="index" :class="['flex relative', msg.isMine ? 'justify-end' : 'justify-start']">
              
              <div v-if="!msg.isMine" class="w-10 h-10 rounded-full flex items-center justify-center text-white font-bold mr-3 shrink-0 shadow-sm mt-2" :style="{ backgroundColor: msg.senderColor }">
                {{ msg.senderName ? msg.senderName.charAt(0) : 'U' }}
              </div>

              <div :class="['flex flex-col max-w-[70%]', msg.isMine ? 'items-end' : 'items-start']">
                <div class="text-xs text-gray-500 mb-1 mx-1 flex items-center gap-2 max-w-full overflow-hidden">
                  <span class="truncate">{{ msg.senderName }}</span>
                  <span v-if="msg.sendTime" class="text-[10px] text-gray-400 font-normal shrink-0">{{ msg.sendTime }}</span>
                </div>
                
                <div :class="['rounded-xl p-3 shadow-sm break-words break-all overflow-hidden', msg.isMine ? 'bg-blue-500 text-white rounded-tr-sm' : 'bg-white text-gray-800 rounded-tl-sm border border-gray-100']">
                  <div v-if="msg.isFile">
                    <template v-if="msg.fileUrl.match(/\.(jpeg|jpg|gif|png|webp|bmp)(\?.*)?$/i)">
                      <img :src="msg.fileUrl" class="max-w-xs max-h-48 rounded cursor-zoom-in border border-gray-200" @click.stop="previewImage(msg.fileUrl)" />
                    </template>
                    <template v-else>
                      <a :href="msg.fileUrl" target="_blank" class="underline flex items-center gap-2 hover:text-blue-200 text-sm overflow-hidden break-words break-all">
                        <File class="w-4 h-4 shrink-0" /> <span class="break-all">{{ msg.fileName }}</span>
                      </a>
                    </template>
                  </div>
                  <span v-else class="break-words break-all whitespace-pre-wrap leading-relaxed">{{ msg.cleanContent }}</span>
                </div>
              </div>

              <div v-if="msg.isMine" class="w-10 h-10 rounded-full flex items-center justify-center text-white font-bold ml-3 shrink-0 shadow-sm mt-2" :style="{ backgroundColor: myInfo.color || '#3b82f6' }">
                {{ myInfo.myname ? myInfo.myname.charAt(0) : '我' }}
              </div>
            </div>
          </div>

          <div class="h-40 bg-white border-t border-gray-200 flex flex-col shrink-0">
            <div class="flex items-center px-4 py-2 space-x-4 text-gray-500">
              <button class="hover:text-gray-800 transition relative" @click="triggerFileUpload">
                <Folder class="w-6 h-6" />
                <input type="file" ref="fileInput" class="hidden" @change="handleFileUpload">
              </button>
            </div>
            <textarea v-model="inputText" @keydown.enter.prevent="sendMessage" class="flex-1 w-full resize-none outline-none px-4 py-2 text-gray-800 bg-transparent" placeholder="输入消息，Enter 发送..."></textarea>
            <div class="flex justify-end px-4 pb-4">
              <button @click="sendMessage" class="bg-blue-500 hover:bg-blue-600 text-white px-6 py-2 rounded font-bold transition">发送</button>
            </div>
          </div>
        </template>
        
        <div v-else class="flex-1 flex items-center justify-center flex-col text-gray-400">
          <MessageCircle class="w-20 h-20 mb-4 opacity-20" />
          <p>选中左侧联系人或打开最近消息开始聊天</p>
        </div>
      </div>
    </div>

    <div v-if="showLogoutConfirmModal" class="fixed inset-0 bg-black bg-opacity-40 flex items-center justify-center z-50">
      <div class="bg-white p-6 rounded-lg shadow-xl w-80 relative flex flex-col items-center">
        <div class="w-12 h-12 bg-red-100 text-red-500 rounded-full flex items-center justify-center mb-4">
          <LogOut class="w-6 h-6" />
        </div>
        <h3 class="text-lg font-bold text-gray-800 mb-2">确认退出登录？</h3>
        <p class="text-sm text-gray-500 text-center mb-6">退出后将无法接收实时消息通知，是否继续？</p>
        <div class="flex w-full gap-3">
          <button @click="showLogoutConfirmModal = false" class="flex-1 py-2 bg-gray-100 text-gray-700 hover:bg-gray-200 rounded transition font-bold text-sm">取消</button>
          <button @click="logout" class="flex-1 py-2 bg-red-500 text-white hover:bg-red-600 rounded transition font-bold text-sm">确认退出</button>
        </div>
      </div>
    </div>

    <div v-if="showFriendSettingsModal" class="fixed inset-0 bg-black bg-opacity-40 flex items-center justify-center z-50">
      <div class="bg-white p-6 rounded-lg shadow-xl w-80 relative flex flex-col">
        <button @click="showFriendSettingsModal = false" class="absolute top-4 right-4 text-gray-400 hover:text-gray-600">✕</button>
        <h3 class="text-xl font-bold mb-4 text-gray-800">好友设置</h3>
        <div class="text-center mb-6">
          <div class="w-16 h-16 rounded-full mx-auto flex items-center justify-center text-white font-bold text-2xl mb-2 shadow" :style="{ backgroundColor: currentContact?.color }">
            {{ currentContact?.myname ? currentContact.myname.charAt(0) : 'F' }}
          </div>
          <div class="font-bold text-gray-800">{{ currentContact?.myname }}</div>
          <div class="text-xs text-gray-500">{{ currentContact?.id }}</div>
        </div>
        <div class="flex flex-col gap-3">
          <button @click="handleFriendAction('black')" class="w-full py-2 bg-orange-50 text-orange-600 hover:bg-orange-100 rounded transition font-bold text-sm">拉入黑名单</button>
          <button @click="handleFriendAction('unblock')" class="w-full py-2 bg-yellow-50 text-yellow-600 hover:bg-yellow-100 rounded transition font-bold text-sm">解除拉黑</button>
          <button @click="handleFriendAction('delfriend')" class="w-full py-2 bg-red-50 text-red-600 hover:bg-red-100 rounded transition font-bold text-sm">删除好友</button>
        </div>
      </div>
    </div>

    <div v-if="showNotifModal" class="fixed inset-0 bg-black bg-opacity-40 flex items-center justify-center z-50">
      <div class="bg-white p-6 rounded-lg shadow-xl w-[28rem] relative flex flex-col max-h-[80vh]">
        <button @click="showNotifModal = false" class="absolute top-4 right-4 text-gray-400 hover:text-gray-600">✕</button>
        <h3 class="text-xl font-bold mb-4 text-gray-800 flex items-center gap-2">
          <Bell class="w-5 h-5 text-blue-500"/> 通知中心
        </h3>
        <div v-if="notifications.length === 0" class="text-center text-gray-400 py-8 text-sm">暂无新通知或验证消息</div>
        <div class="overflow-y-auto flex-1 space-y-3 pr-2">
          <div v-for="(n, idx) in notifications" :key="idx" class="p-3 bg-gray-50 rounded-lg border border-gray-200 shadow-sm">
            <div class="text-sm text-gray-800 mb-3">
              <span v-if="n.use === 'addfriend'">用户 <span class="font-bold text-blue-600">{{ n.account?.replace('frie:', '') }}</span> 请求添加你为好友</span>
              <span v-else-if="n.use === 'addgroup'">用户 <span class="font-bold text-blue-600">{{ n.account?.replace('user:', '') }}</span> 申请加入群聊 <span class="font-bold">{{ n.name?.replace('grop:', '') || n.group }}</span></span>
              <span v-else-if="n.use === 'invitation'">用户 <span class="font-bold text-blue-600">{{ n.account?.replace('user:', '') }}</span> 邀请你加入群聊 <span class="font-bold">{{ n.name?.replace('grop:', '') }}</span></span>
              <span v-else>收到未知的验证请求 ({{ n.use }})</span>
            </div>
            <div class="flex justify-end gap-2">
              <button @click="handleVerify(n, false)" class="px-3 py-1.5 bg-gray-200 text-gray-600 hover:bg-gray-300 rounded text-xs font-bold transition">拒绝</button>
              <button @click="handleVerify(n, true)" class="px-3 py-1.5 bg-blue-500 text-white hover:bg-blue-600 rounded text-xs font-bold transition">同意</button>
            </div>
          </div>
        </div>
      </div>
    </div>

    <div v-if="showGroupSettingsModal" class="fixed inset-0 bg-black bg-opacity-40 flex items-center justify-center z-50">
      <div class="bg-white p-6 rounded-lg shadow-xl w-96 relative flex flex-col max-h-[80vh]">
        <button @click="showGroupSettingsModal = false" class="absolute top-4 right-4 text-gray-400 hover:text-gray-600">✕</button>
        <h3 class="text-xl font-bold mb-4 text-gray-800">群聊设置</h3>
        
        <div v-if="myGroupRank === 'owner' || myGroupRank === 'admin'" class="mb-4 bg-gray-50 p-3 rounded border border-gray-200">
          <label class="block text-xs text-gray-500 mb-2">修改群头像颜色</label>
          <div class="flex gap-2 justify-between">
            <button v-for="c in colorPalette" :key="c" @click="profileForm.newGroupColor = c"
                    :class="['w-6 h-6 rounded-full border-2 transition', profileForm.newGroupColor === c ? 'border-gray-800 scale-125' : 'border-transparent']"
                    :style="{ backgroundColor: c }"></button>
            <button @click="submitGroupColor" class="px-2 bg-blue-50 hover:bg-blue-100 text-blue-600 rounded text-xs font-bold transition">保存</button>
          </div>
        </div>

        <div class="mb-4 bg-gray-50 p-3 rounded border border-gray-200">
          <label class="block text-xs text-gray-500 mb-2">邀请好友入群</label>
          <div class="max-h-32 overflow-y-auto border border-gray-200 rounded p-2 bg-white space-y-1 mb-3">
            <div v-if="inviteableFriends.length === 0" class="text-xs text-gray-400 text-center py-2">暂无可邀请的好友</div>
            <label v-for="friend in inviteableFriends" :key="friend.name" class="flex items-center gap-2 p-1 hover:bg-gray-50 rounded cursor-pointer">
              <input type="checkbox" :value="friend.name.replace('user:', '')" v-model="inviteForm.selectedFriends" class="rounded text-blue-500">
              <div class="w-6 h-6 rounded-full flex items-center justify-center text-white text-xs" :style="{ backgroundColor: friend.color }">{{ friend.myname ? friend.myname.charAt(0) : 'F' }}</div>
              <span class="text-sm text-gray-700">{{ friend.myname }}</span>
            </label>
          </div>
          <button @click="submitInvite" class="w-full px-3 py-2 bg-blue-500 hover:bg-blue-600 text-white rounded text-sm transition font-bold shadow-sm">发送邀请</button>
        </div>

        <div class="text-sm font-bold text-gray-500 mb-2 border-b pb-2">成员列表 ({{ currentGroupMembers.length }} 人)</div>
        <div class="overflow-y-auto flex-1 mb-4 space-y-2">
          <div v-for="member in currentGroupMembers" :key="member.name" class="flex items-center justify-between p-2 hover:bg-gray-50 rounded">
            <div class="flex items-center gap-2">
              <div class="w-8 h-8 rounded-full flex items-center justify-center text-xs text-white" :style="{ backgroundColor: member.color }">
                {{ member.myname ? member.myname.charAt(0) : member.name.replace('user:', '').charAt(0) }}
              </div>
              <span class="text-sm text-gray-700 font-medium">{{ member.myname || member.name.replace('user:', '') }}</span>
              <span v-if="member.rank === 'owner'" class="px-2 py-0.5 bg-yellow-100 text-yellow-700 text-xs rounded">群主</span>
              <span v-else v-if="member.rank === 'admin'" class="px-2 py-0.5 bg-blue-100 text-blue-700 text-xs rounded">管理</span>
            </div>
            
            <div class="flex gap-2" v-if="member.name !== 'user:' + loginForm.account">
              <template v-if="myGroupRank === 'owner'">
                <button v-if="member.rank === 'member'" @click="handleGroupCmd('upl', member.name)" class="text-xs text-blue-500 hover:underline">升管</button>
                <button v-if="member.rank === 'admin'" @click="handleGroupCmd('dnl', member.name)" class="text-xs text-orange-500 hover:underline">降管</button>
                <button @click="handleGroupCmd('del', member.name)" class="text-xs text-red-500 hover:underline">踢出</button>
              </template>
              <template v-else-if="myGroupRank === 'admin'">
                <button v-if="member.rank === 'member'" @click="handleGroupCmd('del', member.name)" class="text-xs text-red-500 hover:underline">踢出</button>
              </template>
            </div>
          </div>
        </div>
        <div class="pt-4 border-t">
          <button @click="handleQuitGroup" class="w-full py-2 bg-red-50 text-red-600 hover:bg-red-100 rounded transition font-bold text-sm">
            {{ myGroupRank === 'owner' ? '解散群聊' : '退出群聊' }}
          </button>
        </div>
      </div>
    </div>

    <div v-if="showAddModal" class="fixed inset-0 bg-black bg-opacity-40 flex items-center justify-center z-50">
      <div class="bg-white p-6 rounded-lg shadow-xl w-96">
        <h3 class="text-xl font-bold mb-4 text-gray-800">添加 / 创建</h3>
        <select v-model="addForm.type" class="w-full mb-4 p-2 border border-gray-300 rounded focus:outline-none focus:border-blue-500">
          <option v-if="currentTab === 'friends' || currentTab === 'chat'" value="add_friend">添加好友</option>
          <option v-if="currentTab === 'groups' || currentTab === 'chat'" value="join_group">加入群聊</option>
          <option v-if="currentTab === 'groups' || currentTab === 'chat'" value="create_group">创建群聊</option>
        </select>
        <input v-if="addForm.type === 'add_friend'" v-model="addForm.target" placeholder="输入对方纯账号 (无需user:)" class="w-full mb-4 p-2 border border-gray-300 rounded focus:outline-none focus:border-blue-500">
        <input v-if="addForm.type === 'join_group'" v-model="addForm.target" placeholder="输入群聊纯名称 (无需grop:)" class="w-full mb-4 p-2 border border-gray-300 rounded focus:outline-none focus:border-blue-500">
        
        <div v-if="addForm.type === 'create_group'" class="space-y-3">
          <input v-model="addForm.target" placeholder="输入新群聊名称 (必填)" class="w-full p-2 border border-gray-300 rounded focus:outline-none focus:border-blue-500">
          <div class="text-sm font-bold text-gray-700 mt-2">选择邀请的好友：</div>
          <div class="max-h-40 overflow-y-auto border border-gray-200 rounded p-2 bg-gray-50 space-y-1">
            <div v-if="creatableFriends.length === 0" class="text-xs text-gray-400 text-center py-2">暂无好友可邀请</div>
            <label v-for="friend in creatableFriends" :key="friend.name" class="flex items-center gap-2 p-1 hover:bg-gray-100 rounded cursor-pointer">
              <input type="checkbox" :value="friend.name.replace('user:', '')" v-model="addForm.selectedFriends" class="rounded text-blue-500">
              <div class="w-6 h-6 rounded-full flex items-center justify-center text-white text-xs" :style="{ backgroundColor: friend.color }">{{ friend.myname ? friend.myname.charAt(0) : 'F' }}</div>
              <span class="text-sm text-gray-700">{{ friend.myname }}</span>
            </label>
          </div>
        </div>
        <div class="flex justify-end gap-3 mt-4">
          <button @click="showAddModal = false" class="px-4 py-2 text-gray-600 hover:bg-gray-100 rounded transition">取消</button>
          <button @click="submitAdd" class="px-4 py-2 bg-blue-500 hover:bg-blue-600 text-white rounded transition">发送请求</button>
        </div>
      </div>
    </div>

    <div v-if="showProfileModal" class="fixed inset-0 bg-black bg-opacity-40 flex items-center justify-center z-50">
      <div class="bg-white p-8 rounded-lg shadow-xl w-96 relative">
        <button @click="showProfileModal = false" class="absolute top-4 right-4 text-gray-400 hover:text-gray-600">✕</button>
        
        <h3 class="text-2xl font-bold mb-6 text-center text-gray-800">我的主页</h3>
        
        <div class="flex flex-col items-center mb-6">
          <div class="w-20 h-20 rounded-full flex items-center justify-center text-white font-bold text-3xl mb-3 shadow" :style="{ backgroundColor: myInfo.color || '#3b82f6' }">
            {{ myInfo.myname ? myInfo.myname.charAt(0) : 'U' }}
          </div>
          <div class="text-sm text-gray-500">账号: {{ loginForm.account }}</div>
          <div v-if="myInfo.email" class="text-sm text-gray-500">邮箱: {{ myInfo.email }}</div>
        </div>

        <div class="space-y-4">
          <div class="bg-gray-50 p-3 rounded">
            <label class="block text-xs text-gray-500 mb-2">修改个人头像颜色</label>
            <div class="flex gap-2 justify-between">
              <button v-for="c in colorPalette" :key="c" @click="profileForm.newColor = c"
                      :class="['w-6 h-6 rounded-full border-2 transition', profileForm.newColor === c ? 'border-gray-800 scale-125' : 'border-transparent']"
                      :style="{ backgroundColor: c }"></button>
              <button @click="submitRevise('color')" class="px-2 bg-blue-50 hover:bg-blue-100 text-blue-600 rounded text-xs font-bold transition">保存</button>
            </div>
          </div>

          <div>
            <label class="block text-xs text-gray-500 mb-1">修改昵称</label>
            <div class="flex gap-2">
              <input v-model="profileForm.newName" :placeholder="myInfo.myname || '输入新昵称'" class="flex-1 p-2 border border-gray-300 rounded focus:outline-none focus:border-blue-500 text-sm">
              <button @click="submitRevise('myname')" class="px-3 bg-blue-50 hover:bg-blue-100 text-blue-600 rounded text-sm transition">修改</button>
            </div>
          </div>
          
          <div>
            <label class="block text-xs text-gray-500 mb-1">修改密码</label>
            <div class="flex gap-2">
              <input v-model="profileForm.newPassword" type="password" placeholder="输入新密码" class="flex-1 p-2 border border-gray-300 rounded focus:outline-none focus:border-blue-500 text-sm">
              <button @click="submitRevise('password')" class="px-3 bg-blue-50 hover:bg-blue-100 text-blue-600 rounded text-sm transition">修改</button>
            </div>
          </div>
        </div>

        <div class="mt-8 pt-4 border-t border-gray-100 text-center">
          <button @click="submitDeleteAccount" class="text-red-500 hover:text-red-700 text-sm font-bold transition">⚠️ 注销此账号 (危险操作)</button>
        </div>
      </div>
    </div>

  </div>
</template>

<script setup>
import { ref, reactive, nextTick, computed, onMounted, onUnmounted } from 'vue';
import axios from 'axios';
import { MessageCircle, Users, User, LogOut, Folder, File, Bell, PlusCircle, Settings } from 'lucide-vue-next';

const colorPalette = ['#3b82f6', '#10b981', '#f59e0b', '#ef4444', '#8b5cf6', '#ec4899', '#64748b'];

// [核心修复 1] 强制弹窗队列截断，最多显示2个，防止被发信人刷屏绿框堆叠
const toasts = ref([]);
const showToast = (msg, type = 'success') => {
  const id = Date.now() + Math.random();
  toasts.value.push({ id, msg, type });
  if (toasts.value.length > 2) {
    toasts.value.shift(); // 超过2个直接顶掉最老的
  }
  setTimeout(() => { toasts.value = toasts.value.filter(t => t.id !== id); }, 3000);
};

const parseContent = (rawText, defaultName) => {
  const regex = /^\[(.*?)\]:([\s\S]*)$/;
  const match = rawText.match(regex);
  if (match) {
    const header = match[1]; 
    const msg = match[2];
    const lastDash = header.lastIndexOf('-');
    let t = '', n = defaultName;
    if (lastDash !== -1) {
      t = header.substring(0, lastDash);
      n = header.substring(lastDash + 1);
    } else {
      n = header;
    }
    return { time: t, name: n, clean: msg.trim() };
  }
  return { time: '', name: defaultName, clean: rawText };
};

const getFullTime = () => {
  const now = new Date();
  const yyyy = now.getFullYear();
  const MM = String(now.getMonth() + 1).padStart(2, '0');
  const dd = String(now.getDate()).padStart(2, '0');
  const hh = String(now.getHours()).padStart(2, '0');
  const mm = String(now.getMinutes()).padStart(2, '0');
  return `${yyyy}-${MM}-${dd} ${hh}:${mm}`;
};

const buildPayload = (content) => {
  const t = getFullTime();
  const n = myInfo.myname || loginForm.account;
  return `[${t}-${n}]:${content}`;
};

const isLoggedIn = ref(false);
const isRegisterMode = ref(false);
const loginForm = reactive({ account: '', password: '', email: '', code: '' });
const myInfo = reactive({});

const currentTab = ref('chat');
const currentContact = ref(null);
const currentContactType = ref(''); 

const friendsList = ref([]);
const groupsList = ref([]);
const recentChats = ref([]); 

const inputText = ref('');
const messages = ref([]);
const messageSet = new Set(); 
const messageContainer = ref(null);
const fileInput = ref(null);

let heartbeatTimer = null;
const countdown = ref(0);

const isLoadingHistory = ref(false);
const hasMoreHistory = ref(true);
const isInitialLoad = ref(true);
const historyScrollHeight = ref(0);
const historyStep = ref(1); 
let tempHistoryChunk = [];  
const isFetchingOldHistory = ref(false);

const showAddModal = ref(false);
const addForm = reactive({ type: 'add_friend', target: '', selectedFriends: [] });
const showProfileModal = ref(false);
const profileForm = reactive({ newName: '', newPassword: '', newColor: '', newGroupColor: '' });
const showGroupSettingsModal = ref(false);
const currentGroupMembers = ref([]);
const showFriendSettingsModal = ref(false);
const showNotifModal = ref(false);
const notifications = ref([]);
const showLogoutConfirmModal = ref(false); 

const inviteForm = reactive({ selectedFriends: [] });

const creatableFriends = computed(() => {
  return friendsList.value.filter(f => f.name.replace('user:', '') !== loginForm.account);
});

const inviteableFriends = computed(() => {
  return friendsList.value.filter(f => {
    const isSelf = f.name.replace('user:', '') === loginForm.account;
    const isAlreadyInGroup = currentGroupMembers.value.some(m => m.name.replace('user:', '') === f.name.replace('user:', ''));
    return !isSelf && !isAlreadyInGroup;
  });
});

const showImagePreview = ref(false);
const previewImageUrl = ref('');

const previewImage = (url) => {
  previewImageUrl.value = url;
  showImagePreview.value = true;
};

const submitInvite = () => {
  if (inviteForm.selectedFriends.length === 0) return showToast("请先选择要邀请的好友", "error");
  const pureName = currentContact.value.id.replace('grop:', '');
  
  inviteForm.selectedFriends.forEach(targetUser => {
    ws.send(JSON.stringify({ 
      type: "invite", 
      account: "user:" + loginForm.account, 
      target: "user:" + targetUser, 
      group: "grop:" + pureName 
    }));
  });
  
  showToast("邀请请求已全部发送！", "success");
  inviteForm.selectedFriends = [];
};

const myGroupRank = computed(() => {
  const me = currentGroupMembers.value.find(m => m.name === "user:" + loginForm.account);
  return me ? me.rank : 'member';
});

let ws = null;

const scrollToBottom = (isInitial = false) => {
  nextTick(() => {
    const box = messageContainer.value;
    if (!box) return;

    if (isInitial) {
        box.scrollTop = box.scrollHeight + 9999;
    } else {
        box.scrollTo({ top: box.scrollHeight + 9999, behavior: 'smooth' });
    }

    setTimeout(() => {
      if (messageContainer.value) {
        if (isInitial) {
            messageContainer.value.scrollTop = messageContainer.value.scrollHeight + 9999;
        } else {
            messageContainer.value.scrollTo({ top: messageContainer.value.scrollHeight + 9999, behavior: 'smooth' });
        }
      }
    }, 200);
  });
};

const handleBeforeUnload = () => {
  if (ws && ws.readyState === WebSocket.OPEN && isLoggedIn.value) {
    ws.send(JSON.stringify({ type: "ship", account: "user:" + loginForm.account, mystate: "offline" }));
    ws.close();
  }
};
onMounted(() => { window.addEventListener('beforeunload', handleBeforeUnload); });
onUnmounted(() => { window.removeEventListener('beforeunload', handleBeforeUnload); });

const ensureWebSocket = (onReadyCallback) => {
  if (ws && ws.readyState === WebSocket.OPEN) {
    onReadyCallback();
    return;
  }
  const serverIp = window.location.hostname;
  ws = new WebSocket(`ws://${serverIp}:8081`);
  ws.onopen = () => onReadyCallback();
  ws.onmessage = (event) => {
    try {
      const data = JSON.parse(event.data);

      if (data.type === 'print') {
        if (data.meg && (data.meg.includes('不存在') || data.meg.includes('有误'))) {
            isLoadingHistory.value = false;
            isFetchingOldHistory.value = false;
        }

        if (data.meg === '开始聊天' || data.meg === '以上为历史消息') {
          if (isFetchingOldHistory.value) {
            if (tempHistoryChunk.length === 0) {
              hasMoreHistory.value = false;
            } else {
              messages.value = [...tempHistoryChunk, ...messages.value];
              tempHistoryChunk = [];
            }
          }

          messages.value.sort((a, b) => {
            const timeA = a.sendTime || '';
            const timeB = b.sendTime || '';
            return timeA.localeCompare(timeB);
          });

          isFetchingOldHistory.value = false;
          isLoadingHistory.value = false;

          nextTick(() => {
            if (isInitialLoad.value) {
              isInitialLoad.value = false;
              scrollToBottom(true);
            } else {
              if (messageContainer.value && historyScrollHeight.value > 0) {
                messageContainer.value.scrollTop = messageContainer.value.scrollHeight - historyScrollHeight.value;
                historyScrollHeight.value = 0;
              }
            }
          });
          return;
        }

        const spammyPrints = ['------', '未读消息', '验证消息'];
        if (data.meg && !spammyPrints.some(str => data.meg.includes(str))) {
          showToast(data.meg, data.meg.includes('错误') || data.meg.includes('失败') || data.meg.includes('不够') || data.meg.includes('不存在') || data.meg.includes('有误') ? 'error' : 'success');
        }
        
        if (data.meg === '成功登陆') {
          isLoggedIn.value = true;
          ws.send(JSON.stringify({ type: "ship", account: "user:" + loginForm.account, mystate: "online" }));
          heartbeatTimer = setInterval(() => {
            if (ws && ws.readyState === WebSocket.OPEN && isLoggedIn.value) {
                ws.send(JSON.stringify({ type: "tcp" }));
                // [核心修复 2]：心跳机制中加入通知静默拉取，10秒获取一次，有了消息立刻触发小红点！
                fetchNotifications(true); 
            }
          }, 10000);
          fetchFriends();
          fetchGroups();
          fetchNotifications();
        } else if (data.meg === '注册成功！') {
          isRegisterMode.value = false;
          if (ws) ws.close();
        } else if (data.meg === '修改成功！' || data.meg === '设置成功!') {
          if (profileForm.newName) myInfo.myname = profileForm.newName;
          if (profileForm.newColor) myInfo.color = profileForm.newColor;
          if (profileForm.newGroupColor) fetchGroups();
          profileForm.newName = '';
          profileForm.newPassword = '';
          profileForm.newColor = '';
          profileForm.newGroupColor = '';
        } else if (data.meg === '注销成功') {
          forceLogoutClear();
        } else if (data.meg === '解散成功' || data.meg === '退出成功') {
          showGroupSettingsModal.value = false;
          if (currentContact.value) {
            recentChats.value = recentChats.value.filter(r => r.id !== currentContact.value.id);
          }
          currentContact.value = null;
          fetchGroups();
        } else if (data.meg === '删除成功！' || data.meg === '已拉黑' || data.meg === '已解除拉黑') {
          showFriendSettingsModal.value = false;
          if (currentContact.value && (data.meg === '删除成功！' || data.meg === '已拉黑')) {
             recentChats.value = recentChats.value.filter(r => r.id !== currentContact.value.id);
             currentContact.value = null;
          }
          fetchFriends();
        } else if (data.meg.includes('成为好友') || data.meg.includes('同意') || data.meg.includes('拒绝')) {
          fetchFriends();
          fetchGroups();
        }
      } 
      else if (data.type === 'information') {
        Object.assign(myInfo, data);
      }
      else if (data.type === 'see' && data.see === 'friend') {
        const isOnline = data.mystate && data.mystate.includes('online') ? 'online' : 'offline';
        const cColor = data.color && data.color !== 'null' ? data.color : '#3b82f6';
        const nName = data.myname && data.myname !== 'null' ? data.myname : data.name.replace('user:', '');
        friendsList.value.push({
          name: data.name,
          myname: nName,
          state: isOnline, 
          degree: data.degree,
          color: cColor
        });
        const recent = recentChats.value.find(r => r.id === data.name);
        if (recent && recent.type === 'friend') {
          recent.title = nName;
          recent.avatar = nName.charAt(0);
          recent.color = cColor;
        }
      }
      else if (data.type === 'seegroup') {
        if (['owner', 'admin', 'member'].includes(data.amount)) {
          const cColor = data.color && data.color !== 'null' ? data.color : '#3b82f6';
          const nName = data.myname && data.myname !== 'null' ? data.myname : data.name.replace('user:', '');
          currentGroupMembers.value.push({ name: data.name, rank: data.amount, myname: nName, color: cColor });
        } else {
          const gColor = data.color && data.color !== 'null' ? data.color : '#a855f7';
          groupsList.value.push({ name: data.name, amount: data.amount, color: gColor });
          const recent = recentChats.value.find(r => r.id === data.name);
          if (recent && recent.type === 'group') recent.color = gColor;
        }
      }
      else if (data.type === 'messdata') {
        // [核心修复 3]：更加严苛的防重复过滤器，配合静默轮询确保红点逻辑精确无误
        if (!notifications.value.some(n => n.account === data.account && n.use === data.use && n.name === data.name)) {
          notifications.value.push(data);
        }
      }
      else if (data.type === 'message' || data.type === 'gmessage') {
        const serverAccount = "user:" + loginForm.account;
        
        let isFileMatch = false;
        let extFileUrl = '';
        let extFileName = '';
        
        const senderAcc = data.type === 'gmessage' ? data.account : data.from;
        const isMine = senderAcc === serverAccount;
        
        let rawSName = myInfo.myname || loginForm.account;
        let contactId = '';

        if (data.type === 'gmessage') {
          contactId = data.from;
          rawSName = data.account ? data.account.replace('user:', '') : "群成员";
        } else {
          contactId = isMine ? data.to : data.from;
          if (!isMine) {
            const f = friendsList.value.find(f => "user:" + f.name === data.from);
            rawSName = (f && f.myname) ? f.myname : data.from.replace('user:', '');
          }
        }

        const parsed = parseContent(data.things, rawSName);
        const chatType = data.type === 'gmessage' ? 'group' : 'friend';
        
        if (parsed.clean.startsWith('[文件]') && parsed.clean.includes('http')) {
            isFileMatch = true;
            const splitIndex = parsed.clean.indexOf(' : http');
            if (splitIndex !== -1) {
                extFileName = parsed.clean.substring(4, splitIndex).trim();
                let rawUrl = parsed.clean.substring(splitIndex + 3).trim();
                try {
                    let parsedUrl = new URL(rawUrl);
                    parsedUrl.hostname = window.location.hostname;
                    extFileUrl = parsedUrl.toString();
                } catch (e) {
                    extFileUrl = rawUrl;
                }
            } else {
                extFileUrl = parsed.clean;
                extFileName = '收到的文件';
            }
        }

        const finalSenderName = data.senderName || parsed.name;
        let finalSenderColor = data.senderColor || '#9ca3af';

        if (!data.senderColor) {
           if (isMine) finalSenderColor = myInfo.color || '#3b82f6';
           else {
             const mMatch = currentGroupMembers.value.find(m => m.name === senderAcc);
             if (mMatch) finalSenderColor = mMatch.color;
             else {
               const fMatch = friendsList.value.find(f => f.name === senderAcc);
               if (fMatch) finalSenderColor = fMatch.color;
             }
           }
        }
        
        if (!isFetchingOldHistory.value && !isLoadingHistory.value) {
            let recent = recentChats.value.find(r => r.id === contactId);
            if (!recent) {
              let displayTitle = contactId.replace('user:', '').replace('grop:', '');
              let chatColor = chatType === 'group' ? '#a855f7' : '#3b82f6';

              if (chatType === 'friend') {
                const f = friendsList.value.find(f => "user:" + f.name === contactId);
                if (f && f.myname) displayTitle = f.myname;
                if (f && f.color) chatColor = f.color;
              } else {
                const g = groupsList.value.find(g => "grop:" + g.name === contactId);
                if (g && g.color) chatColor = g.color;
              }

              recent = { id: contactId, type: chatType, title: displayTitle, avatar: displayTitle.charAt(0), lastMsg: '', time: '', unread: 0, color: chatColor };
              recentChats.value.unshift(recent);
            } else {
              recentChats.value = [recent, ...recentChats.value.filter(r => r.id !== contactId)];
            }
            recent.lastMsg = parsed.clean;
            recent.time = parsed.time || getFullTime();
            
            const isCurrentWindow = currentContactType.value === chatType && currentContact.value?.id === contactId;
            if (!isMine && !isCurrentWindow) {
              recent.unread += 1;
            }
        }

        const isCurrentWindow = currentContactType.value === chatType && currentContact.value?.id === contactId;
        
        if (isCurrentWindow) {
          const finalMsgId = data.msgId || (parsed.clean + parsed.time + finalSenderName);
          if (!messageSet.has(finalMsgId)) {
            messageSet.add(finalMsgId);
            const msgObj = {
              isMine: isMine,
              senderName: finalSenderName,
              senderColor: finalSenderColor,
              sendTime: parsed.time,
              cleanContent: parsed.clean,
              isFile: isFileMatch,
              fileUrl: isFileMatch ? extFileUrl : '',
              fileName: isFileMatch ? extFileName : ''
            };

            if (isFetchingOldHistory.value) {
              tempHistoryChunk.push(msgObj);
            } else {
              messages.value.push(msgObj);
              if (!isLoadingHistory.value) {
                 scrollToBottom(false); 
              }
            }
          }
        } else {
          if (!isLoadingHistory.value && !isFetchingOldHistory.value && !isMine) {
            showToast(`收到来自 ${finalSenderName} 的新消息`);
          }
        }
      }
    } catch (e) {
      console.error('解析消息失败:', event.data);
    }
  };

  ws.onclose = () => { isLoggedIn.value = false; if (heartbeatTimer) clearInterval(heartbeatTimer); };
  ws.onerror = () => { showToast("无法连接到网关！", "error"); };
};

const sendVerifyCode = () => {
  if (!loginForm.email) return showToast('请先输入邮箱', 'error');
  const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
  if (!emailRegex.test(loginForm.email)) return showToast('邮箱格式不正确', 'error');

  countdown.value = 60;
  const timer = setInterval(() => {
    countdown.value--;
    if (countdown.value <= 0) clearInterval(timer);
  }, 1000);

  ensureWebSocket(() => {
    ws.send(JSON.stringify({ type: "getpwd", return: "register", account: loginForm.email }));
  });
};

const handleSubmit = () => {
  if (!loginForm.account || !loginForm.password) return showToast('请输入账号和密码', 'error');
  if (loginForm.account.startsWith('user:')) return showToast('直接输入账号即可，无需输入 user:', 'error');
  
  if (isRegisterMode.value) {
    if (!loginForm.email) return showToast('请输入绑定的邮箱', 'error');
    const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
    if (!emailRegex.test(loginForm.email)) return showToast('邮箱格式不正确', 'error');
    if (!loginForm.code) return showToast('请输入验证码', 'error');
  }

  ensureWebSocket(() => {
    const serverAccount = "user:" + loginForm.account;
    const req = isRegisterMode.value ? {
        type: "register", account: serverAccount, password: loginForm.password,
        qqEmail: loginForm.email, myname: loginForm.account, code: loginForm.code 
    } : {
        type: "login", account: serverAccount, password: loginForm.password
    };
    ws.send(JSON.stringify(req));
  });
};

// [新增] 静默拉取参数，确保红点轮询时不会造成页面内闪烁
const fetchNotifications = (isSilent = false) => {
  if (!isSilent) notifications.value = [];
  ws.send(JSON.stringify({ type: "messdata", account: "mess:user:" + loginForm.account }));
  ws.send(JSON.stringify({ type: "messdata", account: "mess:grop:" + loginForm.account }));
};

const fetchFriends = () => {
  friendsList.value = []; 
  ws.send(JSON.stringify({ type: "see", see: "friend", account: "frie:" + loginForm.account }));
};

const fetchGroups = () => {
  groupsList.value = []; 
  ws.send(JSON.stringify({ type: "seegroup", rank: "all", account: "mygp:" + loginForm.account }));
};

const switchTab = (tab) => {
  currentTab.value = tab;
  fetchFriends();
  fetchGroups();
};

const selectContactFromRecent = (chat) => {
  chat.unread = 0;
  selectContact({ id: chat.id, name: chat.id, myname: chat.title, color: chat.color }, chat.type);
};

const selectContact = (contact, type) => {
  const rawId = contact.id || contact.name;
  const pureName = rawId.replace('user:', '').replace('grop:', '');
  const normalizedId = (type === 'group' ? 'grop:' : 'user:') + pureName;

  currentContact.value = { ...contact, id: normalizedId, rawName: contact.name };
  currentContactType.value = type;
  
  messages.value = []; 
  messageSet.clear();
  tempHistoryChunk = [];
  
  historyStep.value = 1;
  isLoadingHistory.value = true;
  hasMoreHistory.value = true;
  isInitialLoad.value = true;
  isFetchingOldHistory.value = false;
  historyScrollHeight.value = 0;
  
  const recent = recentChats.value.find(r => r.id === normalizedId);
  if (recent) recent.unread = 0;

  if (type === 'friend') {
    ws.send(JSON.stringify({ type: "chat", account: "user:" + loginForm.account, name: "user:" + pureName }));
  } else if (type === 'group') {
    ws.send(JSON.stringify({ type: "gchat", account: "mygp:" + loginForm.account, name: "grop:" + pureName }));
    currentGroupMembers.value = [];
    ws.send(JSON.stringify({ type: "seegroup", rank: "all", account: "grop:" + pureName }));
  }
  
  setTimeout(() => {
      isLoadingHistory.value = false;
      isFetchingOldHistory.value = false;
      messages.value.sort((a, b) => (a.sendTime || '').localeCompare(b.sendTime || ''));
      if(isInitialLoad.value) {
          isInitialLoad.value = false;
          scrollToBottom(true);
      }
  }, 3000);
};

const handleChatScroll = (e) => {
  if (e.target.scrollTop === 0 && !isLoadingHistory.value && hasMoreHistory.value && !isFetchingOldHistory.value) {
    loadFullHistory();
  }
};

const loadFullHistory = () => {
  historyStep.value += 1;
  historyScrollHeight.value = messageContainer.value ? messageContainer.value.scrollHeight : 0;
  isLoadingHistory.value = true;
  isFetchingOldHistory.value = true;
  tempHistoryChunk = []; 

  const pureName = currentContact.value.id.replace('user:', '').replace('grop:', '');
  
  if (currentContactType.value === 'friend') {
    const a = loginForm.account;
    const b = pureName;
    const key = "read:" + (a > b ? `${b}:${a}` : `${a}:${b}`);
    ws.send(JSON.stringify({ type: "history", account: key, step: historyStep.value }));
  } else {
    ws.send(JSON.stringify({ type: "history", account: "grop:" + pureName, name: "user:" + loginForm.account, step: historyStep.value }));
  }

  setTimeout(() => {
      if (isFetchingOldHistory.value) {
          if (tempHistoryChunk.length > 0) {
              messages.value = [...tempHistoryChunk, ...messages.value];
              tempHistoryChunk = [];
          }
          isFetchingOldHistory.value = false;
      }
      messages.value.sort((a, b) => (a.sendTime || '').localeCompare(b.sendTime || ''));
      isLoadingHistory.value = false;
  }, 3000);
};

const sendMessage = () => {
  if (!inputText.value.trim() || !ws || !currentContact.value) return;
  const isGroup = currentContactType.value === 'group';
  const pureTarget = currentContact.value.id.replace('user:', '').replace('grop:', ''); 
  const serverAccount = "user:" + loginForm.account;

  const payload = buildPayload(inputText.value);
  const myCurrentName = myInfo.myname || loginForm.account;
  const myCurrentColor = myInfo.color || '#3b82f6';
  const msgId = Date.now() + '-' + Math.random().toString(36).substr(2, 9);

  if (isGroup) {
    ws.send(JSON.stringify({
      type: "gmessage", account: serverAccount, from: "grop:" + pureTarget, things: payload,
      senderName: myCurrentName, senderColor: myCurrentColor, msgId: msgId
    }));
  } else {
    ws.send(JSON.stringify({
      type: "message", from: serverAccount, to: "user:" + pureTarget, things: payload,
      senderName: myCurrentName, senderColor: myCurrentColor, msgId: msgId
    }));
  }
  
  const parsedTime = getFullTime();
  messageSet.add(msgId);

  messages.value.push({ 
    isMine: true, 
    senderName: myCurrentName,
    senderColor: myCurrentColor,
    sendTime: parsedTime,
    cleanContent: inputText.value, 
    isFile: false 
  });
  
  let recent = recentChats.value.find(r => r.id === currentContact.value.id);
  if (!recent) {
    recent = { 
        id: currentContact.value.id, 
        type: currentContactType.value, 
        title: currentContact.value.myname || currentContact.value.id.replace('user:', '').replace('grop:', ''), 
        avatar: (currentContact.value.myname || currentContact.value.id.replace('user:', '').replace('grop:', '')).charAt(0), 
        lastMsg: inputText.value, 
        time: parsedTime, 
        unread: 0, 
        color: currentContact.value.color || (currentContactType.value === 'group' ? '#a855f7' : '#3b82f6') 
    };
    recentChats.value.unshift(recent);
  } else {
    recent.lastMsg = inputText.value;
    recent.time = parsedTime;
    recentChats.value = [recent, ...recentChats.value.filter(r => r.id !== currentContact.value.id)];
  }
  
  inputText.value = '';
  scrollToBottom(false); 
};

const openAddModal = () => {
  showAddModal.value = true;
  addForm.target = '';
  addForm.selectedFriends = [];
  if (currentTab.value === 'friends') addForm.type = 'add_friend';
  else if (currentTab.value === 'groups') addForm.type = 'join_group';
};

const submitAdd = () => {
  if (!addForm.target) return showToast('目标名称不能为空', 'error');
  const pureTarget = addForm.target.replace('user:', '').replace('grop:', '');

  if (addForm.type === 'add_friend') {
    ws.send(JSON.stringify({ type: "add", name: pureTarget, account: "frie:" + loginForm.account }));
  } else if (addForm.type === 'join_group') {
    ws.send(JSON.stringify({ type: "add", name: pureTarget, account: "mygp:" + loginForm.account }));
  } else if (addForm.type === 'create_group') {
    const req = { type: "creategroup", name: pureTarget, account: "user:" + loginForm.account, count: addForm.selectedFriends.length.toString() };
    addForm.selectedFriends.forEach((m, idx) => { req[`member${idx}`] = "user:" + m; });
    ws.send(JSON.stringify(req));
  }
  showAddModal.value = false;
};

const openNotifModal = () => {
  fetchNotifications();
  showNotifModal.value = true;
};

const handleVerify = (notif, isAccept) => {
  const req = { type: "verify", pass: notif.use, result: isAccept ? "yes" : "no", account: "user:" + loginForm.account };
  if (notif.use === 'addfriend') req.name = notif.account.replace('frie:', '');
  else if (notif.use === 'addgroup') { req.group = notif.name; req.name = notif.account.replace('mygp:', ''); }
  else if (notif.use === 'invitation') req.name = notif.name.replace('grop:', '');

  ws.send(JSON.stringify(req));
  notifications.value = notifications.value.filter(n => n !== notif);
};

const handleFriendAction = (type) => {
  if (!currentContact.value) return;
  const pureTarget = currentContact.value.id.replace('user:', '');
  const req = { type: type, account: "user:" + loginForm.account, name: "user:" + pureTarget };
  if (type === 'unblock') { req.type = 'black'; req.return = "1"; }
  ws.send(JSON.stringify(req));
};

const openGroupSettings = () => {
  showGroupSettingsModal.value = true;
  currentGroupMembers.value = [];
  const pureName = currentContact.value.id.replace('grop:', '');
  ws.send(JSON.stringify({ type: "seegroup", rank: "all", account: "grop:" + pureName }));
};

const handleGroupCmd = (useCmd, targetUser) => {
  const pureName = currentContact.value.id.replace('grop:', '');
  ws.send(JSON.stringify({ type: "cmd", use: useCmd, name: "grop:" + pureName, account: "user:" + loginForm.account, target: targetUser }));
};

const handleQuitGroup = () => {
  const pureName = currentContact.value.id.replace('grop:', '');
  ws.send(JSON.stringify({ type: "delgroup", name: pureName, account: "user:" + loginForm.account }));
};

const submitGroupColor = () => {
  if (!profileForm.newGroupColor) return showToast("请先选择颜色", "error");
  const pureName = currentContact.value.id.replace('grop:', '');
  ws.send(JSON.stringify({ type: "revise", account: "user:" + loginForm.account, groupColor: profileForm.newGroupColor, groupName: pureName }));
};

const submitRevise = (field) => {
  const req = { type: "revise", account: "user:" + loginForm.account };
  if (field === 'myname') {
    if (!profileForm.newName) return showToast("新昵称不能为空", "error");
    req.myname = profileForm.newName;
  } else if (field === 'password') {
    if (!profileForm.newPassword) return showToast("新密码不能为空", "error");
    req.password = profileForm.newPassword;
  } else if (field === 'color') {
    if (!profileForm.newColor) return showToast("请先选择颜色", "error");
    req.color = profileForm.newColor;
  }
  ws.send(JSON.stringify(req));
};

const submitDeleteAccount = () => {
  if (confirm("确定要永久注销该账号吗？此操作无法恢复！")) {
    ws.send(JSON.stringify({ type: "delete", account: "user:" + loginForm.account }));
  }
};

const triggerFileUpload = () => fileInput.value.click();

const handleFileUpload = async (event) => {
  if (!currentContact.value) return showToast("请先选择聊天对象", "error");
  const file = event.target.files[0];
  if (!file) return;

  const formData = new FormData();
  formData.append('file', file, encodeURIComponent(file.name));

  const serverIp = window.location.hostname;
  
  try {
    const res = await axios.post(`http://${serverIp}:8082/api/upload`, formData, {
      headers: { 'Content-Type': 'multipart/form-data' }
    });

    if (res.data.code === 200) {
      const fileUrl = res.data.data.url;
      const fileName = file.name;
      const isGroup = currentContactType.value === 'group';
      const pureTarget = currentContact.value.id.replace('user:', '').replace('grop:', ''); 
      const serverAccount = "user:" + loginForm.account;
      
      const payload = buildPayload(`[文件] ${fileName} : ${fileUrl}`);
      const myCurrentName = myInfo.myname || loginForm.account;
      const myCurrentColor = myInfo.color || '#3b82f6';
      const msgId = Date.now() + '-' + Math.random().toString(36).substr(2, 9);

      if (isGroup) {
        ws.send(JSON.stringify({ type: "gmessage", account: serverAccount, from: "grop:" + pureTarget, things: payload, senderName: myCurrentName, senderColor: myCurrentColor, msgId: msgId }));
      } else {
        ws.send(JSON.stringify({ type: "message", from: serverAccount, to: "user:" + pureTarget, things: payload, senderName: myCurrentName, senderColor: myCurrentColor, msgId: msgId }));
      }

      messageSet.add(msgId);
      messages.value.push({ 
        isMine: true, 
        senderName: myCurrentName,
        senderColor: myCurrentColor,
        sendTime: getFullTime(),
        cleanContent: `已发送文件: ${fileName}`, 
        isFile: true, 
        fileUrl: fileUrl, 
        fileName: fileName 
      });
      scrollToBottom(false); 
    }
  } catch (err) {
    showToast('文件上传失败', 'error');
  }
  event.target.value = ''; 
};

const handleLogoutClick = () => {
  showLogoutConfirmModal.value = true;
};

const forceLogoutClear = () => {
  showProfileModal.value = false;
  showLogoutConfirmModal.value = false;
  if (ws) {
    ws.send(JSON.stringify({ type: "ship", account: "user:" + loginForm.account, mystate: "offline" }));
    ws.close();
    ws = null;
  }
  if (heartbeatTimer) clearInterval(heartbeatTimer);
  
  isLoggedIn.value = false;
  recentChats.value = [];
  friendsList.value = [];
  groupsList.value = [];
  messages.value = [];
  messageSet.clear();
  currentContact.value = null;
  myInfo.myname = '';
  myInfo.email = '';
  myInfo.color = '';
  loginForm.password = '';
  loginForm.code = '';
};

const logout = () => {
  forceLogoutClear();
};
</script>