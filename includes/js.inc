<?php

/**
 * Base on core drupal_add_js, but support multiple tags
 *
 * @param $tag
 * @param null $data
 * @param string $type
 * @param string $scope
 * @param bool|FALSE $defer
 * @param bool|TRUE $cache
 * @param bool|TRUE $preprocess
 * @param bool|FALSE $reset
 * @return array
 */
function ui_add_js($tag, $data = NULL, $type = 'module', $scope = 'header', $defer = FALSE, $cache = TRUE, $preprocess = TRUE, $reset = FALSE) {
  static $javascript = array();

  if (!isset($javascript[$tag])) {
    $javascript[$tag] = array();
  }

  if (isset($data)) {

    // Add jquery.js and drupal.js, as well as the basePath setting, the
    // first time a Javascript file is added.
    if (empty($javascript[$tag])) {
      $javascript[$tag]['header'] = array(
        'core' => array(),
        'module' => array(),
        'theme' => array(),
        'setting' => array(),
        'inline' => array(),
      );
    }

    if ($reset) {
      $javascript[$tag]['header'] = array(
        'core' => array(
        ),
        'module' => array(),
        'theme' => array(),
        'setting' => array(),
        'inline' => array(),
      );
    }

    if (isset($scope) && !isset($javascript[$tag][$scope])) {
      $javascript[$tag][$scope] = array('core' => array(), 'module' => array(), 'theme' => array(), 'setting' => array(), 'inline' => array());
    }

    if (isset($type) && isset($scope) && !isset($javascript[$tag][$scope][$type])) {
      $javascript[$tag][$scope][$type] = array();
    }

    switch ($type) {
      case 'setting':
        $javascript[$tag][$scope][$type][] = $data;
        break;
      case 'inline':
        $javascript[$tag][$scope][$type][] = array('code' => $data, 'defer' => $defer);
        break;
      default:
        // If cache is FALSE, don't preprocess the JS file.
        $javascript[$tag][$scope][$type][$data] = array('cache' => $cache, 'defer' => $defer, 'preprocess' => (!$cache ? FALSE : $preprocess));
    }
  }

  if (isset($scope)) {

    if (isset($javascript[$tag][$scope])) {
      return $javascript[$tag][$scope];
    }
    else {
      return array();
    }
  }
  else {
    return $javascript[$tag];
  }
}

/**
 * Base on core drupal_get_js, but support multiple tags
 *
 * @param $tag
 * @param string $scope
 * @param null $javascript
 * @return string
 */
function ui_get_js($tag, $scope = 'header', $javascript = NULL) {

  if (!isset($javascript)) {
    $javascript = ui_add_js($tag, NULL, NULL, $scope);
  }

  if (empty($javascript)) {
    return '';
  }

  $output = '';
  $preprocessed = '';
  $no_preprocess = array('core' => '', 'module' => '', 'theme' => '');
  $files = array();
  $preprocess_js = (variable_get('preprocess_js', FALSE) && (!defined('MAINTENANCE_MODE') || MAINTENANCE_MODE != 'update'));
  $directory = file_directory_path();
  $is_writable = is_dir($directory) && is_writable($directory) && (variable_get('file_downloads', FILE_DOWNLOADS_PUBLIC) == FILE_DOWNLOADS_PUBLIC);

  // A dummy query-string is added to filenames, to gain control over
  // browser-caching. The string changes on every update or full cache
  // flush, forcing browsers to load a new copy of the files, as the
  // URL changed. Files that should not be cached (see drupal_add_js())
  // get time() as query-string instead, to enforce reload on every
  // page request.
  $query_string = '?'. variable_get('css_js_query_string', '0');

  // For inline Javascript to validate as XHTML, all Javascript containing
  // XHTML needs to be wrapped in CDATA. To make that backwards compatible
  // with HTML 4, we need to comment out the CDATA-tag.
  $embed_prefix = "\n<!--//--><![CDATA[//><!--\n";
  $embed_suffix = "\n//--><!]]>\n";

  foreach ($javascript as $type => $data) {

    if (!$data) continue;

    switch ($type) {
      case 'setting':
        $output .= '<script type="text/javascript">' . $embed_prefix . 'jQuery.extend(Drupal.settings, ' . drupal_to_js(call_user_func_array('array_merge_recursive', $data)) . ");" . $embed_suffix . "</script>\n";
        break;
      case 'inline':
        foreach ($data as $info) {
          $output .= '<script type="text/javascript"' . ($info['defer'] ? ' defer="defer"' : '') . '>' . $embed_prefix . $info['code'] . $embed_suffix . "</script>\n";
        }
        break;
      default:
        // If JS preprocessing is off, we still need to output the scripts.
        // Additionally, go through any remaining scripts if JS preprocessing is on and output the non-cached ones.
        foreach ($data as $path => $info) {
          if (!$info['preprocess'] || !$is_writable || !$preprocess_js) {
            $no_preprocess[$type] .= '<script type="text/javascript"'. ($info['defer'] ? ' defer="defer"' : '') .' src="'. base_path() . $path . ($info['cache'] ? $query_string : '?'. time()) ."\"></script>\n";
          }
          else {
            $files[$path] = $info;
          }
        }
    }
  }

  // Aggregate any remaining JS files that haven't already been output.
  if ($is_writable && $preprocess_js && count($files) > 0) {
    // Prefix filename to prevent blocking by firewalls which reject files
    // starting with "ad*".
    //$filename = 'js_'. md5(serialize($files) . $query_string) .'.js';

    //@patch for css/js
    //Robbin Chnaged Core to keep the css/js file name same
    //#9346
    // $filename = 'js_'. md5(serialize($files)) .'.js';

    // #14216
    // Change md5 logic to make it sensitive for file change
    // It's better for 7niu CDN
    $filename = 'js_'. ui_files_md5('js', $files) .'.js';

    $preprocess_file = drupal_build_js_cache($files, $filename);

    //SHOULD get querystring again
    $query_string = '?'. variable_get('css_js_query_string', '0');
    $preprocessed .= '<script type="text/javascript" src="'. file_create_url($preprocess_file) . $query_string .'"></script>'."\n";
  }

  // Keep the order of JS files consistent as some are preprocessed and others are not.
  // Make sure any inline or JS setting variables appear last after libraries have loaded.
  $output = $preprocessed . implode('', $no_preprocess) . $output;

  return $output;
}

/**
 * Add theme js resource for specific tag
 *
 * @param $tag
 */
function ui_load_theme_js($tag) {
  global $theme;

  $themes = list_themes();

  $base_theme = array();
  $ancestor = $theme;

  while ($ancestor && isset($themes[$ancestor]->base_theme)) {
    $base_theme[] = $new_base_theme = $themes[$themes[$ancestor]->base_theme];
    $ancestor = $themes[$ancestor]->base_theme;
  }

  $base_theme = array_reverse($base_theme);

  $theme_info = $themes[$theme];

  // Do basically the same as the above for scripts
  $final_scripts = array();

  // Grab scripts from base theme
  foreach ($base_theme as $base) {
    if (!empty($base->scripts)) {
      foreach ($base->scripts as $name => $script) {
        $final_scripts[$name] = $script;
      }
    }
  }

  // Add scripts used by this theme.
  if (!empty($theme_info->scripts)) {
    foreach ($theme_info->scripts as $name => $script) {
      $final_scripts[$name] = $script;
    }
  }

  // Add scripts used by this theme.
  foreach ($final_scripts as $script) {
    ui_add_js($tag, $script, 'theme');
  }
}