<?php

/**
 * Select which theme to use
 *
 * @param $tag
 */
function ui_select($tag = NULL) {
  static $last_tag;

  if (isset($tag)) {
    global $custom_theme, $theme;
    $tags = module_invoke_all('tag_info');
    if (!isset($tags[$tag]) || !$tags[$tag]) {
      return $last_tag;
    }
    $tag_info = $tags[$tag];

    $custom_theme = $tag_info['theme'];

    init_theme();

    ui_include('js');
    ui_include('css');

    ui_load_theme_js($tag);
    ui_load_theme_css($tag);

    switch ($tag) {
      case 'pc':
        ui_add_js($tag, 'misc/drupal.js', 'core');
        break;
    }

    $last_tag = $tag;
  }

  return $last_tag;
}

/**
 * render ui template
 *
 * @param $template
 * @param $data
 * @return mixed
 */
function ui_view($template, $data) {
  return theme('ui', $template, $data);
}

/**
 * 用于基于资源文件列表的内容计算MD5值
 *
 * @param $type
 * @param $data
 * @return string
 */
function ui_files_md5($type, $data) {
  $files_md5 = array();
  switch ($type) {
    case 'js':
      foreach ($data as $path => $info) {
        $files_md5[] = md5_file($path);
      }
      break;

    case 'css':
      foreach ($data as $key => $files) {
        foreach ($files as $file => $preprocess)
          $files_md5[] = md5_file($file);
      }
      break;
  }

  return md5(serialize($files_md5));
}