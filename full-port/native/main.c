// SPDX-License-Identifier: GPL-3.0-or-later
#include <switch.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

static int hex(char c){return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;}
static void save_reply(WebCommonReply* reply){
    char url[8192]={0};size_t size=0;
    if(R_FAILED(webReplyGetLastUrl(reply,url,sizeof(url),&size)))return;
    char* query=strstr(url,"report=");if(!query)return;query+=7;
    char json[8192];size_t n=0;
    while(*query&&n<sizeof(json)-1){
        if(*query=='%'&&query[1]&&query[2]&&hex(query[1])>=0&&hex(query[2])>=0){json[n++]=(char)((hex(query[1])<<4)|hex(query[2]));query+=3;}
        else json[n++]=*query++;
    }
    json[n]=0;
    mkdir("sdmc:/switch/wumingsha-switch",0777);
    FILE* f=fopen("sdmc:/switch/wumingsha-switch/runtime-report.json","w");
    if(f){fwrite(json,1,n,f);fclose(f);printf("Report saved: /switch/wumingsha-switch/runtime-report.json\n");}
    else printf("Report could not be saved. Please capture the diagnostics screen.\n");
}
int main(int argc,char** argv){
    (void)argc;(void)argv;consoleInit(NULL);
    PadState pad;padConfigureInput(1,HidNpadStyleSet_NpadStandard);padInitializeDefault(&pad);
    printf("Wumingsha Switch - full-package runtime validation v0.2.0\n\n");
    printf("This is an engine test, NOT the completed full game.\n");
    printf("A: open offline diagnostics and original-package entry\n");
    printf("+: exit\n\n");
    printf("Requires the installed NSP with HtmlDocument NCA.\n");
    while(appletMainLoop()){
        padUpdate(&pad);u64 down=padGetButtonsDown(&pad);
        if(down&HidNpadButton_Plus)break;
        if(down&HidNpadButton_A){
            WebCommonConfig config;WebCommonReply reply;
            Result rc=webOfflineCreate(&config,WebDocumentKind_OfflineHtmlPage,0,".htdocs/index.html");
            if(R_SUCCEEDED(rc)){
                webConfigSetJsExtension(&config,true);
                webConfigSetWebAudio(&config,true);
                webConfigSetTouchEnabledOnContents(&config,true);
                webConfigSetFooter(&config,true);
                webConfigSetLeftStickMode(&config,WebLeftStickMode_Cursor);
                rc=webConfigShow(&config,&reply);
                if(R_SUCCEEDED(rc)){save_reply(&reply);WebExitReason reason;webReplyGetExitReason(&reply,&reason);printf("Web applet returned. Exit reason: %d\n",reason);}
            }
            printf("Web launch result: 0x%08X\n",rc);
        }
        consoleUpdate(NULL);
    }
    consoleExit(NULL);return 0;
}
