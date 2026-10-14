window.WUMINGSHA_MARK('module',true,'外部 module 脚本实际执行');
import('./module-leaf.js').then(function(module){
    window.WUMINGSHA_MARK('dynamicImport',module.value===42,'动态模块加载完成');
},function(){window.WUMINGSHA_MARK('dynamicImport',false,'动态模块加载失败');});
